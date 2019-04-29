#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <pthread.h>
#include <vector>
#include <algorithm>
#include <unistd.h>

#define CAPACITY 100

using namespace std;

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

struct buf_t {
    int id, cars;
    long ts;
};

struct light_t {
    int id, total;

    bool operator<(const light_t& a) {
        return !(total < a.total);
    }
};

struct hour_t {
    struct tm timeinfo;
    vector<light_t> lights;
    vector<light_t> topN;
};

bool operator==(const tm& a, const tm& b) {
    return (
        a.tm_year == b.tm_year &&
        a.tm_mon == b.tm_mon &&
        a.tm_mday == b.tm_mday &&
        a.tm_hour == b.tm_hour
    );
}

ts_buffer<buf_t> buf(CAPACITY);
vector<hour_t> hours;
fstream fin;
pthread_mutex_t fin_lock;
pthread_mutex_t hours_lock;
int top;

void* producer(void* argv) {

    while (1) {

        pthread_mutex_lock(&fin_lock);
        
        // try to read a line, terminate if no more lines
        string row;
        if (!getline(fin, row)) {
            pthread_mutex_unlock(&fin_lock);
            break;
        }
        
        pthread_mutex_unlock(&fin_lock);

        // read csv data to buf_t struct
        struct buf_t data;
        string col;
        stringstream ss(row);

        getline(ss, col, ',');
        data.id = stoi(col);

        getline(ss, col, ',');
        data.ts = stol(col);

        getline(ss, col);
        data.cars = stoi(col);

        // queue data in buffer
        buf.push(data);
    }
}

void* consumer(void* argv) {

    while (1) {
        // try to get data from buffer
        struct buf_t data;
        bool dataIn = false;

        dataIn = buf.pop(&data);

        if (dataIn) {
            
            // get date and hour from timestamp, clear minutes and seconds
            time_t rawtime = static_cast<time_t>(data.ts);
            struct tm timeinfo = *localtime(&rawtime);
            timeinfo.tm_min = 0;
            timeinfo.tm_sec = 0;

            // begin critical section
            pthread_mutex_lock(&hours_lock);
            
            // find hour collection
            int hr = -1;
            for (int i = 0; i < hours.size(); i++) {
                if (hours[i].timeinfo == timeinfo) {
                    hr = i;
                    break;
                }
            }
            // add new if hour not found
            if (hr == -1) {
                hour_t h;
                h.timeinfo = timeinfo;
                hours.push_back(h);
                hr = hours.size() - 1;
            }
            
            // find traffic light collection
            int lt = -1;
            for (int i = 0; i < hours[hr].lights.size(); i++) {
                if (hours[hr].lights[i].id == data.id) {
                    lt = i;
                    break;
                }
            }
            // add new if traffic light not found
            if (lt == -1) {
                light_t l;
                l.id = data.id;
                hours[hr].lights.push_back(l);
                lt = hours[hr].lights.size() - 1;
            }

            // append sample data
            hours[hr].lights[lt].total += data.cars;
            
            // copy new aggregate data to topN
            hours[hr].topN = vector<light_t>(hours[hr].lights);

            // sort and truncate topN
            sort(hours[hr].topN.begin(), hours[hr].topN.end());
            hours[hr].topN.resize(top);

            // end critical section
            pthread_mutex_unlock(&hours_lock);
            
        }
        else {
            // if no data and input file is empty, terminate
            if (fin.eof()) break;
        }
    }
}

int main(int argc, char** argv) {
    
    // init default args
    int producers = 1;
    int consumers = 1;
    top = 5;

    // read args
    if (argc > 1) producers = atoi(argv[1]);
    if (argc > 2) consumers = atoi(argv[2]);
    if (argc > 3) top = atoi(argv[3]);

    // init pthread refs
    pthread_t p_threads[producers];
    pthread_t c_threads[producers];

    // init mutex
    pthread_mutex_init(&fin_lock, NULL);
    pthread_mutex_init(&hours_lock, NULL);

    // open input file
    fin.open("trafficData.csv");

    // create producers
    for (int i = 0; i < producers; i++) {
        pthread_create(&p_threads[i], NULL, producer, NULL);
    }

    // create consumers
    for (int i = 0; i < consumers; i++) {
        pthread_create(&c_threads[i], NULL, consumer, NULL);
    }

    // wait for producers to join
    for (int i = 0; i < producers; i++) {
        pthread_join(p_threads[i], NULL);
    }

    // wait for consumers to join
    for (int i = 0; i < consumers; i++) {
        pthread_join(c_threads[i], NULL);
    }

    // close input file
    fin.close();

    for (int i = 0; i < hours.size(); i++) {
        printf("%s", asctime(&hours[i].timeinfo));
        printf("--------------------------\n");
        for (int j = 0; j < hours[i].topN.size(); j++) {
            printf("Traffic Light %02d - %d cars.\n", hours[i].topN[j].id, hours[i].topN[j].total);
        }
        printf("\n");
    }

    return 0;
}
