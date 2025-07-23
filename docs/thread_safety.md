# Thread Safety in PocketSphinx

## Random Number Generator Thread Safety

PocketSphinx uses a Mersenne Twister random number generator for features like dithering in audio processing. Prior to version 5.0.4, this RNG used global state and was not thread-safe.

### Thread-Local Storage Support

Starting with version 5.0.4, PocketSphinx supports thread-local storage for the random number generator, making it thread-safe when enabled.

#### Enabling Thread-Local RNG

Thread-local storage is **enabled by default** when building with CMake on supported platforms:

```bash
cmake -DPS_THREAD_LOCAL_RNG=ON ..
```

To explicitly disable it:

```bash
cmake -DPS_THREAD_LOCAL_RNG=OFF ..
```

#### Platform Requirements

Thread-local storage requires one of:
- C11 compiler with `_Thread_local` support
- C++11 compiler with `thread_local` support
- GCC 3.3+ with `__thread` support
- Visual Studio 2015+ with `__declspec(thread)` support

Most modern compilers support thread-local storage.

#### Behavior

When thread-local storage is enabled:
- Each thread maintains its own independent RNG state
- Calling `genrand_seed()` in one thread does not affect other threads
- Sequences generated in different threads are independent
- Thread-safe without any locks or synchronization

When thread-local storage is disabled:
- RNG uses global state (legacy behavior)
- **NOT thread-safe** - concurrent access causes race conditions
- All threads share the same RNG state

#### API Compatibility

The API remains unchanged. Existing code continues to work:

```c
#include <pocketsphinx.h>
#include "util/genrand.h"

/* Seed the RNG */
genrand_seed(12345);

/* Generate random numbers */
long value = genrand_int31();    /* [0, 2^31-1] */
double real = genrand_real3();   /* (0, 1) */
```

#### Where RNG is Used

The random number generator is primarily used for:
- **Audio dithering** in feature extraction (when dithering is enabled)
- Other stochastic processes in speech recognition

Most applications don't need to interact with the RNG directly.

#### Migration Notes

For applications using PocketSphinx in multiple threads:
1. Ensure you're building with `PS_THREAD_LOCAL_RNG=ON` (default)
2. Be aware that each thread now has independent RNG state
3. If you need reproducible sequences across threads, seed each thread explicitly

#### Testing Thread Safety

You can verify thread safety by running:
```bash
./test_genrand_thread_tls
```

This test verifies that:
- Each thread maintains independent RNG state
- No collisions occur between thread sequences
- Deterministic behavior is preserved within each thread