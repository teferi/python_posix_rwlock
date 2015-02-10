import test.test_support
import unittest
thread = test.test_support.import_module('thread')
threading = test.test_support.import_module('threading')

import errno
import posix_rwlock
import select
import socket


GLOBAL_DATA = []


class Waker(object):
    def __init__(self):
        self.writer = socket.socket()
        self.writer.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

        count = 0
        while 1:
            count += 1
            a = socket.socket()
            a.bind(("127.0.0.1", 0))
            a.listen(1)
            connect_address = a.getsockname()  # assigned (host, port) pair
            try:
                self.writer.connect(connect_address)
                break    # success
            except socket.error as detail:
                if (not hasattr(errno, 'WSAEADDRINUSE') or
                        detail[0] != errno.WSAEADDRINUSE):
                    raise
                if count >= 10:  # I've never seen it go above 2
                    a.close()
                    self.writer.close()
                    raise socket.error("Cannot bind trigger!")
                a.close()

        self.reader, addr = a.accept()
        self.reader.setblocking(0)
        self.writer.setblocking(0)
        a.close()
        self.reader_fd = self.reader.fileno()

    def fileno(self):
        return self.reader.fileno()

    def write_fileno(self):
        return self.writer.fileno()

    def close(self):
        self.reader.close()
        self.writer.close()


class RWThread(threading.Thread):
    def __init__(self, lock, name, wakers):
        threading.Thread.__init__(self, name=name)
        self.wakers = wakers
        self.lock = lock
        self.result = ""

    def run(self):
        while True:
            result = ""
            rlist, _, _ = select.select([self.wakers[0].fileno()], [], [])
            operation = self.wakers[0].reader.recv(1)
            # print "'{} {}'".format(self.name, operation),
            if operation == "q" or not operation:
                break
            elif operation == "l":
                self.do_lock()
            elif operation == "t":
                self.do_lock(True)
            elif operation == "u":
                self.unlock()
            elif operation == "d":
                result = self.do()
            elif operation == "n":
                pass
            else:
                raise Exception("Unknown operation")
            if not result:
                result = "_"
            self.wakers[1].writer.send(str(result)[0])

    def unlock(self):
        self.lock.unlock()

    def do_lock(self, *args):
        raise NotImplemented


class Reader(RWThread):

    def do_lock(self, _try=False):
        if _try:
            return self.lock.try_rdlock()
        else:
            return self.lock.rdlock()

    def do(self):
        self.result = len(GLOBAL_DATA)
        return str(self.result)


class Writer(RWThread):
    def do_lock(self, _try=False):
        if _try:
            return self.lock.try_wrlock()
        else:
            return self.lock.wrlock()

    def do(self):
        GLOBAL_DATA.append('1')


class TestRWLock(unittest.TestCase):
    def setUp(self):
        self._threads = test.test_support.threading_setup()

    def tearDown(self):
        test.test_support.threading_cleanup(*self._threads)
        test.test_support.reap_children()

    def test_rwlock(self):
        lock = posix_rwlock.RWLock()
        wakers = {
            'r1': [Waker(), Waker()],
            'r2': [Waker(), Waker()],
            'w1': [Waker(), Waker()],
            'w2': [Waker(), Waker()],
        }
        r1 = Reader(lock=lock, name='r1', wakers=wakers['r1'])
        r2 = Reader(lock=lock, name='r2', wakers=wakers['r2'])
        w1 = Writer(lock=lock, name='w1', wakers=wakers['w1'])
        w2 = Writer(lock=lock, name='w2', wakers=wakers['w2'])

        r1.start()
        r2.start()
        w1.start()
        w2.start()

        # w_chnls['r1'][0].write("d")
        # w_chnls['r1'][0].flush()
        wakers['r1'][0].writer.send('d')
        select.select([wakers['r1'][1].fileno()], [], [])
        result = wakers['r1'][1].reader.recv(1)
        self.assertEqual(r1.result, 0)
        self.assertEqual(result, "0")

        wakers['w1'][0].writer.send('l')
        select.select([wakers['w1'][1].fileno()], [], [])
        result = wakers['w1'][1].reader.recv(1)

        wakers['w1'][0].writer.send('d')
        select.select([wakers['w1'][1].fileno()], [], [])
        result = wakers['w1'][1].reader.recv(1)

        self.assertEqual(len(GLOBAL_DATA), 1)

        wakers['r1'][0].writer.send('l')
        wakers['r1'][0].writer.send('d')
        self.assertEqual(r1.result, 0)

        wakers['w1'][0].writer.send('u')
        select.select([wakers['w1'][1].fileno()], [], [])
        result = wakers['w1'][1].reader.recv(1)
        self.assertEqual(result, '_')
        select.select([wakers['r1'][1].fileno()], [], [])
        result = wakers['r1'][1].reader.recv(2)
        # select.select([wakers['r1'][1].fileno()], [], [])
        # result = wakers['r1'][1].reader.recv(1)
        self.assertEqual(r1.result, 1)

        wakers['w1'][0].writer.send('q')
        wakers['w2'][0].writer.send('q')
        wakers['r1'][0].writer.send('q')
        wakers['r2'][0].writer.send('q')
        w1.join()
        w2.join()
        r1.join()
        r2.join()
