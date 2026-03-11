#ifndef _SNOWFLAKE_H_
#define _SNOWFLAKE_H_

#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/GammaTime.h"
using namespace Gamma;
#include <mutex>
//* 64bit id : 0000  0000  0000  0000  0000  0000  0000  0000  0000  0000  0000  0000  0000  0000  0000  0000
//*            ||                                                           ||     ||         ||            |
//*            ||---------------------时间戳41------------------------------||中心5||--机器7--||--序列号10--|
//*            |
//*          不用
class SnowFlake {
private:
	static const uint64_t start_stmp_ = 1598889600000;
	static const uint64_t sequence_bit_ = 10;
	static const uint64_t machine_bit_ = 7;
	static const uint64_t datacenter_bit_ = 5;

	static const uint64_t max_datacenter_num_ = -1 ^ (uint64_t(-1) << datacenter_bit_);
	static const uint64_t max_machine_num_ = -1 ^ (uint64_t(-1) << machine_bit_);
	static const uint64_t max_sequence_num_ = -1 ^ (uint64_t(-1) << sequence_bit_);

	static const uint64_t machine_left = sequence_bit_;
	static const uint64_t datacenter_left = sequence_bit_ + machine_bit_;
	static const uint64_t timestmp_left = sequence_bit_ + machine_bit_ + datacenter_bit_;

	uint64_t datacenterId;
	uint64_t machineId;
	uint64_t sequence;
	uint64_t lastStmp;

	std::mutex mutex_;

	uint64_t getNextMill() {
		uint64_t mill = Gamma::GetNatureTime();
		while (mill <= lastStmp) {
			mill = Gamma::GetNatureTime();
		}
		return mill;
	}

public:
	bool Init(int datacenter_Id, int machine_Id) {
		if ((uint64_t)datacenter_Id > max_datacenter_num_ || datacenter_Id < 0) {
			return false;
		}
		if ((uint64_t)machine_Id > max_machine_num_ || machine_Id < 0) {
			return false;
		}
		datacenterId = datacenter_Id;
		machineId = machine_Id;
		sequence = 0L;
		lastStmp = 0L;
		return true;
	}

	uint64_t nextId() {
		std::unique_lock<std::mutex> lock(mutex_);
		uint64_t currStmp = Gamma::GetNatureTime();
		if (currStmp < lastStmp) {
			GammaAst(currStmp < lastStmp);
			return 0;
		}

		if (currStmp == lastStmp) {
			sequence = (sequence + 1) & max_sequence_num_;
			if (sequence == 0) {
				currStmp = getNextMill();
			}
		}
		else {
			sequence = 0;
		}
		lastStmp = currStmp;
		return (currStmp - start_stmp_) << timestmp_left
			| datacenterId << datacenter_left
			| machineId << machine_left
			| sequence;
	}
};
#endif //_SNOWFLAKE_H_