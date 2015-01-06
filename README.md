python_posix_rwlock
===================

Python wrapper around posix rwlock.

Sample usage:
```python
>>> import posix_rwlock
>>> lock = posix_rwlock.RWLock()
>>> lock.try_wrlock()
True
>>> lock.wrlock()
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
posix_rwlock.RWLockException: Resource deadlock avoided
>>> lock.unlock()
True
>>> lock.try_rdlock()
True
>>> lock.try_wrlock()
False

```
