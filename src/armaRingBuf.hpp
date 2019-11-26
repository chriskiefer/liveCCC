//
//  armaRingBuf.hpp
//  liveCCC
//
//  Created by Chris Kiefer on 25/11/2019.
//

#ifndef armaRingBuf_hpp
#define armaRingBuf_hpp

#include <armadillo>

using namespace arma;

template <typename T>
class armaRingBuf {
public:
    armaRingBuf() {
        buf.resize(16);
        buf.fill(0);
    }
    armaRingBuf(size_t N) {
        buf.resize(N);
        buf.fill(0);
    }
    
    void push(T x) {
        std::lock_guard<std::mutex> lock(write_mutex);
        buf[idx] = x;
        idx++;
        if (idx==buf.size()) {
            idx=0;
        }
    }
    
    size_t size() {return buf.size();}
    
    Col<T> getBuffer(size_t N) {
        std::lock_guard<std::mutex> lock(write_mutex);
        Col<T> currBuf(N);
        int targidx=0;
        int sizeToEnd = buf.size() - idx;
        if (idx > N) {
            for(int i=idx-N; i < idx; i++, targidx++) {
                currBuf[targidx] = buf[i];
            }
        }else{
            //first chunk
            for(int i=buf.size()-(N-idx); i < buf.size(); i++, targidx++) {
                currBuf[targidx] = buf[i];
            }
            //second chunk
            for(int i=0; i < idx; i++, targidx++) {
                currBuf[targidx] = buf[i];
            }
        }
        return currBuf;
    }
    
private:
    Col<T> buf;
    size_t idx=0;
    std::mutex write_mutex;
};

#endif /* armaRingBuf_hpp */
