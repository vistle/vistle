# Checking file name for extension

You can use `ends_with`:
```
#include <boost/algorithm/string/predicate.hpp>
...

if (boost::algorithm::ends_with(filename, ".ext")) {
    ...
}
```
