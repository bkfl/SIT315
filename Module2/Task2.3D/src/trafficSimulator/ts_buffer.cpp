#include <pthread.h>
#include <unistd.h>

template <class T>
class ts_buffer {
public:
	explicit ts_buffer(int capacity):
		_buf(new T[capacity + 1]),
		_capacity(capacity + 1),
		_tail(0),
		_head(0)
	{
		pthread_mutex_init(&_pushLock, NULL);
		pthread_mutex_init(&_popLock, NULL);
	}
	~ts_buffer()
    {
		delete [] _buf;
	}

	bool push(T item) {

        pthread_mutex_lock(&_pushLock);

        // wait for available capacity
        while (1) {
            int t = _tail;
            if (_isFull(_head, t)) {
                sleep(0.1);
            }
            else {
                break;
            }
        }
        
        _buf[_head] = item;
        
        if (_head == _capacity - 1) _head = 0;
        else _head++;
        
        pthread_mutex_unlock(&_pushLock);
        return true;
    };

	bool pop(T* item) {
	
        pthread_mutex_lock(&_popLock);

        int h = _head;
        if ( _isEmpty(h, _tail) )
        {
            pthread_mutex_unlock(&_popLock);	
            return false;
        }

        *item = _buf[_tail];
        
        if (_tail == _capacity - 1) _tail = 0;
        else _tail++;
        
        pthread_mutex_unlock(&_popLock);
        return true;
    };

private:
	T* _buf;
	int _capacity;
	int _tail;
	int _head;
	pthread_mutex_t _pushLock;
	pthread_mutex_t _popLock;

    bool _isEmpty(int h, int t) {
	
	    return (h - t) == 0;
    };

    bool _isFull(int h, int t) {
	
        if (h > t) t += _capacity;
        return (t - h) == 1;
    };

};