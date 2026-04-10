add_executable(
    hamarc
    main.cpp
    cli.cpp
    archive.cpp
    hamming_codec.cpp
)
target_compile_features(hamarc PRIVATE cxx_std_20)
