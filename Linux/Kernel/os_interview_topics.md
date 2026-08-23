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
