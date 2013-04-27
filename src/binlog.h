#ifndef SSDB_BINLOG_H_
#define SSDB_BINLOG_H_

#include "include.h"
#include <string>
#include "leveldb/db.h"
#include "leveldb/options.h"
#include "leveldb/slice.h"
#include "leveldb/status.h"
#include "leveldb/write_batch.h"
#include "util/lock.h"
#include "util/bytes.h"


class Binlog{
	private:
		std::string buf;
		static const unsigned int HEADER_LEN = sizeof(uint64_t) + 2;
	public:
		Binlog(){}
		Binlog(uint64_t seq, char type, char cmd, const leveldb::Slice &key);
		
		int load(const leveldb::Slice &s);

		uint64_t seq() const;
		char type() const;
		char cmd() const;
		const Bytes key() const;

		const char* data() const{
			return buf.data();
		}
		int size() const{
			return (int)buf.size();
		}
		const std::string repr() const{
			return this->buf;
		}
		std::string dumps() const;
};

// circular queue
class BinlogQueue{
	private:
		leveldb::DB *db;
		uint64_t min_seq;
		uint64_t last_seq;
		uint64_t tran_seq;
		int capacity;
		leveldb::WriteBatch batch;
	public:
		Mutex mutex;

		BinlogQueue(leveldb::DB *db);
		~BinlogQueue();
		
		void begin();
		void rollback();
		leveldb::Status commit();
		// leveldb put
		void Put(const leveldb::Slice& key, const leveldb::Slice& value);
		// leveldb delete
		void Delete(const leveldb::Slice& key);
		void log(char type, char cmd, const leveldb::Slice &key);
		void log(char type, char cmd, const std::string &key);
		
		/** @returns
		 1 : log.seq greater than or equal to seq
		 0 : not found
		 -1: error
		 */
		int find_next(uint64_t seq, Binlog *log) const;
		int find_last(Binlog *log) const;
};

class Transaction{
private:
	BinlogQueue *logs;
public:
	Transaction(BinlogQueue *logs){
		this->logs = logs;
		logs->mutex.lock();
		logs->begin();
	}
	
	~Transaction(){
		logs->rollback();
		logs->mutex.unlock();
	}
};


#endif
