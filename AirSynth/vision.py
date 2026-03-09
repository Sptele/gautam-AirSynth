from hmac import new
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
print(pyd_path)
sys.path.append(pyd_path)

import AirSynth as syn
print(dir(syn))

"""
API Requirements for the Synth

The Synth must provide:
- a function to create and add new waveforms
    - These waveforms show be a harmonic series, specifically
- etc...


"""

class ROperations(Enum):
    NOTHING = 1
    NEW = 2
    CONTINUE = 3
    DIE = 4

@dataclass
class MPFrameContext:
    frame_bgr: any
    frame_index: int

@dataclass
class PinchState:
    """State for a pinch gesture interaction (e.g., left-hand pinch)."""
    active: bool = False
    base_pos: tuple[float, float] | None = None
    base_freqs: list[float] = None
    base_gains: list[float] = None

    def __post_init__(self):
        # Avoid mutable default args
        if self.base_freqs is None:
            self.base_freqs = []
        if self.base_gains is None:
            self.base_gains = []


@dataclass
class TrackingData:
    hands_open: tuple[bool, bool] # l, r
    frequencies: list[float]
    gains: list[float]
    gain: float
    play: bool

    # Left-hand pinch state
    l_pinch: PinchState | None = PinchState()

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

    freq_inc: float | None = None
    gain_inc: float | None = None

    def compute_increments(self, h: float, w: float, base_pos: tuple[float, float]):
        self.freq_inc = (self.max_freq - self.default_freq) / (h - base_pos[1] * h)
        self.gain_inc = (self.max_gain - self.default_gain) / (w - base_pos[0] * w)

    def reset_increments(self):
        # reset instance fields
        self.freq_inc = None
        self.gain_inc = None

    def clamp_freq(self, freq: float) -> float:
        return max(self.min_freq, min(freq, self.max_freq))

    def clamp_gain(self, gain: float) -> float:
        return max(self.min_gain, min(gain, self.max_gain))

class CameraHandler:
    def __init__(self, model_path, bounds: TrackingDataBounds=TrackingDataBounds(440, 0, 2093, 0.25, 0, 2, True), TRACKER_NUM=9):
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

        self.pending: dict[int, MPFrameContext] = {}

        self.tracking_data = TrackingData((True, True), [], [], 0.0, True)

        self.bounds = bounds
        self.TRACKER_NUM = TRACKER_NUM

        self.synth = syn.SynthAPI()


    def on_camera_frame(self, result: vision.HandLandmarkerResult, output_image: mp.Image, timestamp_ms: int):
        ctx = self.pending.pop(timestamp_ms, None)
        if ctx is None:
            return

        self.with_curr_frame(result, ctx.frame_bgr, ctx.frame_index)

    def with_curr_frame(self, result: vision.HandLandmarkerResult, frame: mp.Image, timestamp_ms: int):
        if result is None:
            return

        if result.hand_landmarks:
            h, w, c = frame.shape

            # Draw each landmark as a filled circle.
            for hand_idx, landmarks in enumerate(result.hand_landmarks):
                if hand_idx == 1:
                    """
                    LEFT HAND
                    """
                    self.compute_left_hand(landmarks, w, h)

                    r_color = 255 if not self.tracking_data.hands_open[0] else 0
                  
                    # Alternate colors per hand for easier visual inspection.
                    color = (r_color, 0, 255)

                    x = int(landmarks[self.TRACKER_NUM].x * w)
                    y = int(landmarks[self.TRACKER_NUM].y * h)

                    cv2.circle(frame, (x, y), 3, color, thickness=12)

                    INDEX_TIP, THUMB_TIP = 8, 4

                    DETECTION_RADIUS = 25
                    r_color = 255 if self.index_thumb_closed(landmarks, DETECTION_RADIUS, w, h) else 0

                    color = (r_color, 255, 255)

                    x = int(landmarks[INDEX_TIP].x * w)
                    y = int(landmarks[INDEX_TIP].y * h)

                    cv2.circle(frame, (x, y), 3, color, thickness=4)

                    x = int(landmarks[THUMB_TIP].x * w)
                    y = int(landmarks[THUMB_TIP].y * h)

                    cv2.circle(frame, (x, y), 3, color, thickness=4)

                    

                if hand_idx == 0:
                    """
                    RIGHT HAND
                    """
                    curr_op = self.compute_right_hand(landmarks, w, h)

                    r_color = 255 if not self.tracking_data.hands_open[1] else 0
                    b_color = 255 if curr_op == ROperations.DIE else 0

                    # Alternate colors per hand for easier visual inspection.
                    color = (r_color, 255, b_color)

                    x = int(landmarks[self.TRACKER_NUM].x * w)
                    y = int(landmarks[self.TRACKER_NUM].y * h)

                    cv2.circle(frame, (x, y), 3, color, thickness=12)

                    if not self.tracking_data.hands_open[1]: # If right hand is currently closed
                        # Show Frequency, Gain
                        cv2.putText(frame, f"{self.tracking_data.new_freq:.1f}hz", (x+35, y-15), cv2.FONT_HERSHEY_SIMPLEX, 1, (0,255,0), 2, cv2.LINE_AA)
                        cv2.putText(frame, f"{self.tracking_data.new_gain*100:.0f}%", (x+35, y+15), cv2.FONT_HERSHEY_SIMPLEX, 1, (0,255,0), 2, cv2.LINE_AA)


        cv2.imshow("AirSynth", frame)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            self.cap.release()
            cv2.destroyAllWindows()
            self.pending.clear()

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
        hand_cl = self.is_hand_closed(lm, w, h)

        return_flag = ROperations.NOTHING

        # Open -> Closed - Add to Tracking Data
        if self.tracking_data.hands_open[1] and hand_cl: 
            self.tracking_data.new_freq = self.bounds.clamp_freq(440) # New Frequency
            self.tracking_data.new_gain = self.bounds.clamp_gain(0.25) # New Gain

            self.tracking_data.new_base_pos = (lm[self.TRACKER_NUM].x, lm[self.TRACKER_NUM].y)

            self.bounds.compute_increments(h, w, self.tracking_data.new_base_pos)

            return_flag = ROperations.NEW

        # Closed -> Closed - Update Current New Freq/Gain Values
        elif not self.tracking_data.hands_open[1] and hand_cl:
            if self.tracking_data.new_base_pos is not None and self.bounds.freq_inc is not None and self.bounds.gain_inc is not None:
                curr_x, curr_y = lm[self.TRACKER_NUM].x, lm[self.TRACKER_NUM].y

                diff_y = -(curr_y - self.tracking_data.new_base_pos[1])
                diff_x = -(curr_x - self.tracking_data.new_base_pos[0])

                self.tracking_data.new_freq = self.bounds.clamp_freq(self.bounds.default_freq + diff_y * h * self.bounds.freq_inc)
                self.tracking_data.new_gain = self.bounds.clamp_gain(self.bounds.default_gain - diff_x * w * self.bounds.gain_inc)

                return_flag = ROperations.CONTINUE
       
        # Closed -> Open - Remove from Tracking Data, Append to Permanent List
        elif not self.tracking_data.hands_open[1] and not hand_cl: 
            freq, gain = self.tracking_data.pop()

            self.synth.add_waveform_series(freq, gain)

            self.bounds.reset_increments()

            return_flag = ROperations.DIE

        # Open -> Open - Passive State
        elif self.tracking_data.hands_open[1] and not hand_cl: 
            pass # No-Op

        # Always update hands
        self.tracking_data.hands_open = (self.tracking_data.hands_open[0], not hand_cl)

        return return_flag

    
    def index_thumb_closed(self, lm, detection_radius: float, w, h):
        index_tip, thumb_tip = 8, 4
        return (fabs(lm[index_tip].x - lm[thumb_tip].x) * w < detection_radius and
                fabs(lm[index_tip].y - lm[thumb_tip].y) * h < detection_radius)

    def compute_left_hand(self, lm, w, h):
        # In the left hand, a fist **toggles** pause/play
        # Pinching (index+thumb) lets you adjust ALL gains & ALL freqs
        hand_cl = self.is_hand_closed(lm, w, h, True)

        # First frame of fist only (edge trigger)
        if self.tracking_data.hands_open[0] and hand_cl:
            self.tracking_data.play = not self.tracking_data.play

        DETECTION_RADIUS = 25
        pinching = self.index_thumb_closed(lm, DETECTION_RADIUS, w, h)

        ps = self.tracking_data.l_pinch

        # When pinching: move up/down to shift ALL freqs; move left/right to shift ALL gains.
        if pinching:
            pinch_pos = (lm[self.TRACKER_NUM].x, lm[self.TRACKER_NUM].y)

            # Pinch start: capture baseline once
            if not ps.active:
                ps.active = True
                ps.base_pos = pinch_pos
                ps.base_freqs = list(self.tracking_data.frequencies)
                ps.base_gains = list(self.tracking_data.gains)

                # If there is an in-progress (new) freq/gain, include them too
                if self.tracking_data.new_freq is not None:
                    ps.base_freqs.append(self.tracking_data.new_freq)
                if self.tracking_data.new_gain is not None:
                    ps.base_gains.append(self.tracking_data.new_gain)

                if ps.base_pos is not None:
                    self.bounds.compute_increments(h, w, ps.base_pos)

            # Pinch continue: apply deltas relative to baseline
            if ps.base_pos is not None and self.bounds.freq_inc is not None and self.bounds.gain_inc is not None:
                curr_x, curr_y = pinch_pos
                base_x, base_y = ps.base_pos

                diff_y = -(curr_y - base_y)
                diff_x = -(curr_x - base_x)

                freq_delta = diff_y * h * self.bounds.freq_inc
                gain_delta = -diff_x * w * self.bounds.gain_inc

                if ps.base_freqs:
                    self.tracking_data.frequencies = [
                        self.bounds.clamp_freq(f + freq_delta)
                        for f in ps.base_freqs[:len(self.tracking_data.frequencies)]
                    ]

                if ps.base_gains:
                    self.tracking_data.gains = [
                        self.bounds.clamp_gain(g + gain_delta)
                        for g in ps.base_gains[:len(self.tracking_data.gains)]
                    ]

                # Also apply to the in-progress right-hand values (if present)
                if self.tracking_data.new_freq is not None and len(ps.base_freqs) >= (len(self.tracking_data.frequencies) + 1):
                    self.tracking_data.new_freq = self.bounds.clamp_freq(ps.base_freqs[-1] + freq_delta)
                if self.tracking_data.new_gain is not None and len(ps.base_gains) >= (len(self.tracking_data.gains) + 1):
                    self.tracking_data.new_gain = self.bounds.clamp_gain(ps.base_gains[-1] + gain_delta)

        else:
            # Pinch end: clear state
            if ps.active:
                ps.active = False
                ps.base_pos = None
                ps.base_freqs = []
                ps.base_gains = []
                self.bounds.reset_increments()

        # Always update left-hand open/closed state
        self.tracking_data.hands_open = (not hand_cl, self.tracking_data.hands_open[1])


    def spin(self):
        with vision.HandLandmarker.create_from_options(self.options) as landmarker:
            frame_index = 0

            while self.cap.isOpened():
                good, frame_bgr = self.cap.read()

                if not good or frame_bgr is None:
                    print("Failed to read frame from camera. Exiting...")
                    break

                # MediaPipe Image expects RGB.
                frame_rgb = cv2.cvtColor(frame_bgr, cv2.COLOR_BGR2RGB)
                mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=frame_rgb)

                # LIVE_STREAM requires monotonically increasing timestamps (ms).
                timestamp_ms = int(time.time() * 1000)

                # If frames arrive within same ms, ensure uniqueness.
                while timestamp_ms in self.pending:
                    timestamp_ms += 1

                self.pending[timestamp_ms] = MPFrameContext(
                    frame_bgr=frame_bgr, frame_index=frame_index
                )

                landmarker.detect_async(mp_image, timestamp_ms)

                frame_index += 1


HAND_TASK_MODEL_PATH = "hand_landmarker.task"


def main():
    cam = CameraHandler(HAND_TASK_MODEL_PATH)

    cam.spin()


    
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
