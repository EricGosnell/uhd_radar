.PHONY: test
test: build-cxx cxx-test python-test

.PHONY: build-cxx
build-cxx:
	@echo "Building C++ project..."
	cmake -S sdr -B sdr/build
	cmake --build sdr/build

.PHONY: cxx-test
cxx-test:
	@echo "Running C++ tests..."
	ctest --test-dir sdr/build --output-on-failure -E gpsLock

.PHONY: python-test
python-test:
	@echo "Running Python tests..."
	conda run -n uhd pytest tests/
