set(tests
    test_gjk_libccd-inl_epa.cpp
    test_gjk_libccd-inl_extractClosestPoints.cpp
    test_gjk_libccd-inl_signed_distance.cpp
)

# Build all the tests
foreach(test ${tests})
  add_fcl_test(${test})
endforeach(test)
