CXXFLAGS = -std=c++17 -Wall -Wextra -I../include -I. -DUNIT_TEST
SANFLAGS = -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1
DEFINES =

SOURCES = test_main.cpp test_config.cpp test_logger.cpp test_http_framing.cpp test_byte_search.cpp test_json_safe_access.cpp ../ConfigManager.cpp ../HttpFraming.cpp ../ByteSearch.cpp ../JsonSafeAccess.cpp
TARGET = run_tests

all: $(TARGET)
	./$(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(DEFINES) $^ -o $@

# Same suite, built with ASan+UBSan - catches the class of bug (buffer
# overrun, use-after-free, UB) that a plain host build won't. Run this before
# trusting any change to firmware/common/.
asan: $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(DEFINES) $^ -o run_tests_asan
	./run_tests_asan

clean:
	rm -f $(TARGET) run_tests_asan
