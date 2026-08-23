
## Priority inversion: 

Suppose there are three tasks:

H – High priority
M – Medium priority
L – Low priority

```text
L acquires lock
       ↓
H blocks waiting for lock
       ↓
M preempts L
       ↓
H waits until M finishes
       ↓
L resumes, releases lock
       ↓
H finally runs



```