# Tests CMakeLists.txt
# This file is included only when BUILD_TESTS is ON
# Copyright 2024 SimpleDaemons
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#     http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if(BUILD_TESTS)
    # Add test executables here when they are implemented
    # For now, this is just a placeholder
    
    # Example test structure:
    # add_executable(test_configuration test_configuration.cpp)
    # target_link_libraries(test_configuration simple_rsyncd)
    # add_test(NAME ConfigurationTest COMMAND test_configuration)
    
    message(STATUS "Tests directory configured (no tests implemented yet)")
endif()
