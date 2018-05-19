Name: Tanakol, Ahmet

Matriculation number: 03691681
# Slotted Pages

Implement the *schema segment*, the *free-space inventory* and the *slotted
pages*. In the schema segment, you should store the segment ids of the
free-space inventory and the slotted page segment as well as the total number of
existing slotted pages and the schema string. The slotted page segment should
store slotted pages as discussed in the lecture. Your implementation should
support allocation, reading, writing, resizing, and erasing of records in a
page. You should also implement TID redirects and use a free-space inventory
to find suitable pages more efficiently.

`include/moderndbs/schema.h` contains a simplified schema that you should be
able to serialize and deserialize into the schema segment. For this purpose we
also added the library *rapidjson* (`vendor/rapidjson.cmake`) that you could
use.

Important files for this assignment:
- `include/moderndbs/schema.h`
- `include/moderndbs/segment.h`
- `include/moderndbs/slotted_page.h`
- `src/fsi_segment.cc`
- `src/schema.cc`
- `src/schema_segment.cc`
- `src/slotted_page.cc`
- `src/sp_segment.cc`

Additionally, your code will be checked for code quality. You can run the
checks for that by using the `lint` CMake target (e.g. by running `make lint`).
When your code does not pass those checks, we may deduct (a small amount of)
points.

**IMPORTANT**: You can find tests in `test/segment_test.cc`. Note, however,
that the tests are only meant to help YOU find the most obvious bugs in your
code. This time, the complexity of the assigment does not come from algorithmic
problems but the higher amount of C++ code that you must write. 
We tried to keep the interface generic which gives you a high degree of freedom
during the implementation at the cost of more fine-granular tests.
Therefore, DO NOT blindly assume that your implementation is correct & complete
once your tests turn green.
Instead, we encourage you to add/modify/delete the tests throughout the
assigments.
