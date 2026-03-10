from math import fabs
import time
from dataclasses import dataclass
from enum import Enum

import cv2
import mediapipe as mp
from mediapipe.tasks.python import vision

import sys
import os

root = os.path.dirname(__file__)
pyd_path = os.path.abspath( os.path.join(root, "..", "x64", "Debug") )
sys.path.append(pyd_path)

import AirSynth as syn

class ROperations(Enum):
    NOTHING = 1
    NEW = 2
    CONTINUE = 3
    DIE = 4
    ERROR = 5

@dataclass
class MPFrameContext:
    frame_bgr: any
    frame_index: int

# AI-Guided
@dataclass
class PinchState:
    """State for a pinch gesture interaction (e.g., left-hand pinch)."""
    active: bool = False
    base_pos: tuple[float, float] | None = None
    
    new_length: float | None = None

@dataclass
class TrackingData:
    hands_open: tuple[bool, bool] # l, r
    frequencies: list[float]
    gains: list[float]
    gain: float
    play: bool
    length: float

    # Left-hand pinch state
    l_pinch: PinchState | None = PinchState()

    # Right-hand debounce (closed/open stability)
    r_closed_frames: int = 0
    r_open_frames: int = 0
    r_new_armed: bool = True
    r_is_closed_stable: bool = False

    new_freq: float | None = None
    new_gain: float | None = None
    new_base_pos: tuple[float, float] | None = None

    def pop(self) -> tuple[float, float]:
        rtnr = (self.new_freq, self.new_gain)

        if self.new_freq is not None:
            self.frequencies.append(self.new_freq)

        if self.new_gain is not None:
            self.gains.append(self.new_gain)

        self.new_freq = None
        self.new_gain = None
        self.new_base_pos = None

        return rtnr

@dataclass
class TrackingDataBounds:
    default_freq: float
    min_freq: float
    max_freq: float

    default_gain: float
    min_gain: float
    max_gain: float

    default_length: float
    min_length: float 
    max_length: float

    freq_inc: float | None = None
    gain_inc: float | None = None
    length_inc: float | None = None

    def compute_l_increment(self, w: float, h: float, base_pos: tuple[float, float]):
        top_px = base_pos[1] * h
        bot_px = h * (1.0 - base_pos[1])

        top_px = max(1.0, top_px)
        bot_px = max(1.0, bot_px)
        
        c_y_px = max(top_px, bot_px)

        top = 0

        if c_y_px == top_px:
            top = self.max_length - self.default_length
        else:
            top = self.default_length - self.min_length

        self.length_inc = top / c_y_px

    def compute_increments(self, h: float, w: float, base_pos: tuple[float, float]):
        top_px = base_pos[1] * h
        bot_px = h * (1.0 - base_pos[1])
        left_px = base_pos[0] * w
        right_px = w * (1.0 - base_pos[0])

        c_y_px = max(top_px, bot_px)
        c_x_px = max(left_px, right_px)

        top_freq = 0
        top_gain = 0

        if c_y_px == top_px:
            # Top was chosen, top = M -d
            top_freq = self.max_freq - self.default_freq
        else: top_freq = self.default_freq - self.min_freq

        if c_x_px == right_px:
            top_gain = self.max_gain - self.default_gain
        else: top_gain = self.default_gain - self.min_gain

        self.freq_inc = top_freq / c_y_px
        self.gain_inc = top_gain / c_x_px

    def reset_increments(self):
        # reset instance fields
        self.freq_inc = None
        self.gain_inc = None
    
    def reset_l_increment(self):
        self.length_inc = None

    def clamp_freq(self, freq: float) -> float:
        return max(self.min_freq, min(freq, self.max_freq))

    def clamp_gain(self, gain: float) -> float:
        return max(self.min_gain, min(gain, self.max_gain))

    def clamp_length(self, length: float) -> float:
        return max(self.min_length, min(length, self.max_length))

class CameraHandler:
    def __init__(self, model_path, bounds: TrackingDataBounds=TrackingDataBounds(440, 0, 2093, 0.25, 0, 2, 3, 1, 60), TRACKER_NUM=9):
        base_options = mp.tasks.BaseOptions(model_asset_path=model_path)
        vision_running_mode = mp.tasks.vision.RunningMode

        self.options = vision.HandLandmarkerOptions(
            base_options=base_options,
            running_mode=vision.RunningMode.LIVE_STREAM,
            min_hand_detection_confidence=0.5,
            min_hand_presence_confidence=0.5,
            min_tracking_confidence=0.5,
            result_callback=self.on_camera_frame,
            num_hands=2
        )

        self.cap = cv2.VideoCapture(0)
        self.frame_length = 33

        # LLM-guided detail I didn't even think of, but which is smart
        self.pending: dict[int, MPFrameContext] = {}

        self.tracking_data = TrackingData((True, True), [], [], 0.0, True, bounds.default_length)

        self.bounds = bounds
        self.TRACKER_NUM = TRACKER_NUM

        self.synth = syn.SynthAPI()
        self.synth.start()

        self.running = True

    # "running" flag was the idea of LLM
    def request_stop(self):
        if not self.running:
            return

        self.running = False

        try:
            self.synth.stop()
        finally:
            if self.cap is not None:
                self.cap.release()
            self.pending.clear()

    def on_camera_frame(self, result: vision.HandLandmarkerResult, output_image: mp.Image, timestamp_ms: int):
        ctx = self.pending.pop(timestamp_ms, None)
        if ctx is None:
            return

        self.with_curr_frame(result, ctx.frame_bgr, ctx.frame_index)

    def with_curr_frame(self, result: vision.HandLandmarkerResult, frame: mp.Image, timestamp_ms: int):
        if not self.running or result is None:
            return

        h, w, c = frame.shape

        # For some reason, I always struggle to flip the frame (in this project and my past one)
        # So, I googled it and the Google AI thing showed me this
        display_frame = cv2.flip(frame, 1)

        if result.hand_landmarks:

            # AI-guided
            def mx(x_px): return (w - 1) - x_px

            for hand_idx, landmarks in enumerate(result.hand_landmarks):
                handedness = None

                if result.handedness and hand_idx < len(result.handedness) and result.handedness[hand_idx]:
                    handedness = result.handedness[hand_idx][0].category_name

                if not handedness or handedness not in ("Left", "Right"):
                    handedness = "Right" if hand_idx == 0 else "Left"

                if handedness == "Left":
                    """
                    LEFT HAND
                    """
                    self.compute_left_hand(landmarks, w, h)

                    r_color = 255 if not self.tracking_data.hands_open[0] else 0
                  
                    color = (r_color, 0, 255)

                    x = int(landmarks[self.TRACKER_NUM].x * w)
                    y = int(landmarks[self.TRACKER_NUM].y * h)

                    cv2.circle(display_frame, (mx(x), y), 3, color, thickness=12)

                    INDEX_TIP, THUMB_TIP = 20, 4

                    DETECTION_RADIUS = 25
                    tc = self.index_thumb_closed(landmarks, DETECTION_RADIUS, w, h)
                    r_color = 255 if tc else 0

                    color = (r_color, 255, 255)

                    x = int(landmarks[INDEX_TIP].x * w)
                    y = int(landmarks[INDEX_TIP].y * h)

                    cv2.circle(display_frame, (mx(x), y), 3, color, thickness=4)

                    x = int(landmarks[THUMB_TIP].x * w)
                    y = int(landmarks[THUMB_TIP].y * h)

                    cv2.circle(display_frame, (mx(x), y), 3, color, thickness=4)

                    if tc and self.tracking_data.l_pinch.new_length is not None:
                        cv2.putText(display_frame, f"{self.tracking_data.l_pinch.new_length:.1f}s", (mx(x) + 35, y), cv2.FONT_HERSHEY_SIMPLEX, 1, (0,255,0), 2, cv2.LINE_AA)


                if handedness == "Right":
                    """
                    RIGHT HAND
                    """
                    curr_op = self.compute_right_hand(landmarks, w, h)

                    if curr_op == ROperations.ERROR:
                        cv2.circle(display_frame, (mx(x), y), 3, (255, 0, 0), thickness = 16)

                        continue

                    r_color = 255 if not self.tracking_data.hands_open[1] else 0
                    b_color = 255 if curr_op == ROperations.DIE else 0

                    color = (r_color, 255, b_color)

                    x = int(landmarks[self.TRACKER_NUM].x * w)
                    y = int(landmarks[self.TRACKER_NUM].y * h)

                    cv2.circle(display_frame, (mx(x), y), 3, color, thickness=12)


                    if (not self.tracking_data.hands_open[1] and
                        self.tracking_data.new_freq is not None and
                        self.tracking_data.new_gain is not None):  # If right hand is currently closed

                        # Show Frequency, Gain
                        cv2.putText(display_frame, f"{self.tracking_data.new_freq:.1f}hz", (mx(x)+35, y-15), cv2.FONT_HERSHEY_SIMPLEX, 1, (0,255,0), 2, cv2.LINE_AA)
                        cv2.putText(display_frame, f"{self.tracking_data.new_gain*100:.0f}%", (mx(x)+35, y+15), cv2.FONT_HERSHEY_SIMPLEX, 1, (0,255,0), 2, cv2.LINE_AA)


        cv2.imshow("AirSynth", display_frame)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            self.request_stop()            

            return

    def is_hand_closed(self, lm, w, h, left_hand=False):
        # determine if hand is closed
        tips = [8, 12, 16, 20]
        knuckles = [7, 11, 15, 19]

        for i in range(4): # Main four fingers
            curr_tip = tips[i]
            curr_knuckle = knuckles[i]

            if lm[curr_tip].y < lm[curr_knuckle].y:
                return False

        t_tip, t_knuckle = 4, 3
        if (left_hand and lm[t_tip].x * w <= lm[t_knuckle].x * w) or (not left_hand and lm[t_tip].x * w >= lm[t_knuckle].x * w):
            return False


        return True

    def compute_right_hand(self, lm, w, h) -> ROperations:
        # In attempting to figure out how to fix jitters, I stumbled upon the concept of debounce
        # The Google AI & Copilot helped explain the implementation for this a lot

        # Debounce thresholds
        CLOSE_DEBOUNCE_FRAMES = 10
        OPEN_REARM_FRAMES = 5

        hand_cl = self.is_hand_closed(lm, w, h)

        # update debounce counters (raw state)
        if hand_cl:
            self.tracking_data.r_closed_frames += 1
            self.tracking_data.r_open_frames = 0
        else:
            self.tracking_data.r_open_frames += 1
            self.tracking_data.r_closed_frames = 0

        # When open has been stable long enough, re-arm and clear stable-closed latch
        if self.tracking_data.r_open_frames >= OPEN_REARM_FRAMES:
            self.tracking_data.r_new_armed = True
            self.tracking_data.r_is_closed_stable = False

        # Determine whether we have debounced into a stable closed state
        closed_stable_now = hand_cl and (self.tracking_data.r_closed_frames >= CLOSE_DEBOUNCE_FRAMES)

        return_flag = ROperations.NOTHING

        # Trigger NEW exactly once when we first become stably closed
        if closed_stable_now and not self.tracking_data.r_is_closed_stable and self.tracking_data.r_new_armed:
            self.tracking_data.r_new_armed = False
            self.tracking_data.r_is_closed_stable = True

            self.tracking_data.new_freq = self.bounds.clamp_freq(self.bounds.default_freq)
            self.tracking_data.new_gain = self.bounds.clamp_gain(self.bounds.default_gain)

            self.tracking_data.new_base_pos = (lm[self.TRACKER_NUM].x, lm[self.TRACKER_NUM].y)
            self.bounds.compute_increments(h, w, self.tracking_data.new_base_pos)

            # Consider the hand "closed" for state machine purposes from this point
            self.tracking_data.hands_open = (self.tracking_data.hands_open[0], False)
            return ROperations.NEW

        # If we're in stable-closed mode, update the current values
        if self.tracking_data.r_is_closed_stable and hand_cl:
            if (self.tracking_data.new_base_pos is not None and
                self.bounds.freq_inc is not None and self.bounds.gain_inc is not None and
                self.tracking_data.new_freq is not None and self.tracking_data.new_gain is not None):

                
                curr_x, curr_y = lm[self.TRACKER_NUM].x, lm[self.TRACKER_NUM].y

                diff_y = -(curr_y - self.tracking_data.new_base_pos[1])
                diff_x = -(curr_x - self.tracking_data.new_base_pos[0])

                self.tracking_data.new_freq = self.bounds.clamp_freq(self.bounds.default_freq + diff_y * h * self.bounds.freq_inc)
                self.tracking_data.new_gain = self.bounds.clamp_gain(self.bounds.default_gain - diff_x * w * self.bounds.gain_inc)

                return_flag = ROperations.CONTINUE

        # Commit on release (stable closed -> open)
        if self.tracking_data.r_is_closed_stable and not hand_cl:
            if self.tracking_data.new_freq is not None and self.tracking_data.new_gain is not None:
                freq, gain = self.tracking_data.pop()

                if freq > 0 and gain > 0 and self.tracking_data.length:
                    try:
                        self.synth.add_waveform_series(freq, gain, self.tracking_data.length)
                    except e:
                        return_flag = ROperations.ERROR


            self.bounds.reset_increments()

            if return_flag != ROperations.ERROR:
                return_flag = ROperations.DIE

        self.tracking_data.hands_open = (self.tracking_data.hands_open[0], not hand_cl)

        return return_flag

    
    def index_thumb_closed(self, lm, detection_radius: float, w, h):
        index_tip, thumb_tip = 20, 4
        return (fabs(lm[index_tip].x - lm[thumb_tip].x) * w < detection_radius and
                fabs(lm[index_tip].y - lm[thumb_tip].y) * h < detection_radius)

    def compute_left_hand(self, lm, w, h):
        # In the left hand, a fist **toggles** pause/play
        # Pinch Logic is based off of Right Hand logic with some AI assistance
        
        hand_cl = self.is_hand_closed(lm, w, h, True)

        # First frame of fist (edge trigger)
        if self.tracking_data.hands_open[0] and hand_cl:
            self.tracking_data.play = not self.tracking_data.play

            if not self.tracking_data.play:
                self.synth.stop()
            else:
                self.synth.start()

        DETECTION_RADIUS = 25
        pinching = self.index_thumb_closed(lm, DETECTION_RADIUS, w, h)

        ps = self.tracking_data.l_pinch

        if pinching:
            pinch_pos = (lm[self.TRACKER_NUM].x, lm[self.TRACKER_NUM].y)

            if not ps.active: # Pinch Start
                ps.active = True
                ps.base_pos = pinch_pos

                self.tracking_data.length = self.bounds.default_length

                self.bounds.compute_l_increment(w, h, ps.base_pos)



            else: # Pinch Continue
                diff_y = ps.base_pos[1] - pinch_pos[1]

                if self.bounds.length_inc:
                    ps.new_length = self.bounds.clamp_length(self.bounds.default_length + diff_y * h * self.bounds.length_inc)

        else:
            if ps.active: # Pinch End
                ps.active = False
                ps.base_pos = None
                
                self.tracking_data.length = ps.new_length
                
                if self.tracking_data.length:
                    self.synth.rebuild_with_new_length(float(self.tracking_data.length))

                ps.new_length = None


        # Always update left-hand open/closed state
        self.tracking_data.hands_open = (not hand_cl, self.tracking_data.hands_open[1])


    def spin(self):
        try:
            with vision.HandLandmarker.create_from_options(self.options) as landmarker:
                frame_index = 0

                while self.cap.isOpened():
                    good, frame_bgr = self.cap.read()

                    if not good or frame_bgr is None:
                        print("Failed to read frame from camera. Exiting...")
                        break

                    frame_rgb = cv2.cvtColor(frame_bgr, cv2.COLOR_BGR2RGB)
                    mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=frame_rgb)

                    timestamp_ms = int(time.time() * 1000)

                    while timestamp_ms in self.pending:
                        timestamp_ms += 1

                    self.pending[timestamp_ms] = MPFrameContext(
                        frame_bgr=frame_bgr, frame_index=frame_index
                    )

                    landmarker.detect_async(mp_image, timestamp_ms)

                    frame_index += 1
        finally: # End
            self.running = False
            
            try: self.synth.stop()
            except Exception: pass

            if self.cap is not None:
                self.cap.release()

            cv2.destroyAllWindows()
            self.pending.clear()


HAND_TASK_MODEL_PATH = "hand_landmarker.task"

def main():
    cam = CameraHandler(HAND_TASK_MODEL_PATH)

    cam.spin()
    
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
