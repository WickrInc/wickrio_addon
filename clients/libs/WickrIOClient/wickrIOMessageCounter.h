#ifndef WICKRIOMESSAGECOUNTER
#define WICKRIOMESSAGECOUNTER

#define NUM_SECONDS 240

class WickrIOMessageCounter {
    int m_seconds[NUM_SECONDS + 1];
    int m_curSecond;

public:
    WickrIOMessageCounter() : m_curSecond(0) {
        clear();
    }

    void clear() {
        for (int i=0; i<NUM_SECONDS+1; i++)
            m_seconds[i] = 0;
        m_curSecond = 0;
    }

    int getCumulative(int seconds) {
        int curSec = m_curSecond;
        if (seconds > NUM_SECONDS)
            seconds = NUM_SECONDS;
        int retCount = 0;
        while (seconds > 0) {
            retCount += m_seconds[curSec];
            if (--curSec < 0)
                curSec = NUM_SECONDS;
            --seconds;
        }
        return retCount;
    }

    int add(int numApps) {
        m_seconds[m_curSecond] += numApps;
        return m_seconds[m_curSecond];
    }

    void incSecond() {
        if (++m_curSecond > NUM_SECONDS)
            m_curSecond = 0;
        m_seconds[m_curSecond] = 0;
    }
};

#endif // WICKRIOMESSAGECOUNTER

