#include <stdlib.h>

void __assert_fail (const char *assertion, const char *file, unsigned int line, const char *function){
  //__assert_fail_base (_("%s%s%s:%u: %s%sAssertion `%s' failed.\n%n"), assertion, file, line, function);
  abort();
}
