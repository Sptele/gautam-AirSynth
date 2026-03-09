from nanobind.build import build_extension

sources = ["SynthAPI", "ComplexWave", "Sine", "ADSREnvelope", "Wave"]

h_sources = [f"{el}.h" for el in sources]
c_sources = [f"{el}.cpp" for el in sources]

sources = h_sources + c_sources

build_extension(
    name="SynthAPI", 
    sources=sources,
    extra_compile_args=["/O2"]
)