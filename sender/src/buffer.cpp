/*****************************************************************************
Copyright (c) 2001 - 2011, The Board of Trustees of the University of Illinois.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

 * Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

 * Redistributions in binary form must reproduce the
  above copyright notice, this list of conditions
  and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

 * Neither the name of the University of Illinois
  nor the names of its contributors may be used to
  endorse or promote products derived from this
  software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

/*****************************************************************************
written by
   Yunhong Gu, last updated 03/12/2011
 *****************************************************************************/

#include <cstring>
#include <cmath>
#include "buffer.h"

using namespace std;

CSndBuffer::CSndBuffer(const int& size, const int& mss):
		m_BufLock(),
		m_pBlock(NULL),
		m_pFirstBlock(NULL),
		m_pCurrBlock(NULL),
		m_pLastBlock(NULL),
		m_pBuffer(NULL),
		m_iNextMsgNo(1),
		m_iSize(size),
		m_iMSS(mss),
		m_iCount(0)
{
	// initial physical buffer of "size"
	m_pBuffer = new Buffer;
	m_pBuffer->m_pcData = new char [m_iSize * m_iMSS];
	m_pBuffer->m_iSize = m_iSize;
	m_pBuffer->m_pNext = m_pBuffer;
	bufferEnder = m_pBuffer;

	// circular linked list for out bound packets
	m_pBlock = new Block;
	m_pBuffer->first_block = m_pBlock;
	Block* pb = m_pBlock;
	int bufferNo = 0;
	for (int i = 1; i < m_iSize; ++ i)
	{
		pb->m_pNext = new Block;
		pb->m_iBufferNo = bufferNo;
		pb = pb->m_pNext;
		bufferNo++;
	}
	pb->m_pNext = m_pBlock;
	pb->m_iBufferNo = bufferNo;
	blockEnder = pb;

	pb = m_pBlock;
	char* pc = m_pBuffer->m_pcData;
	for (int i = 0; i < m_iSize; ++ i)
	{
		pb->m_pcData = pc;
		pb = pb->m_pNext;
		pc += m_iMSS;
	}

	m_pFirstBlock = m_pCurrBlock = m_pLastBlock = m_pBlock;
	FirstBuffer = m_pBuffer;
	FirstBlock = 0;
/*	Block *test = m_pBlock;
	do {
		cout << "initial " << test->m_iBufferNo << " " << test << endl;
		test = test->m_pNext;
	} while (test != m_pBlock);*/

#ifndef WIN32
pthread_mutex_init(&m_BufLock, NULL);
#else
m_BufLock = CreateMutex(NULL, false, NULL);
#endif
}

CSndBuffer::~CSndBuffer()
{
	Block* pb = m_pBlock->m_pNext;
	while (pb != m_pBlock)
	{
		Block* temp = pb;
		pb = pb->m_pNext;
		delete temp;
	}
	delete m_pBlock;
	m_pBuffer = FirstBuffer->m_pNext;
	while (m_pBuffer != FirstBuffer)
	{
		Buffer* temp = m_pBuffer;
		m_pBuffer = m_pBuffer->m_pNext;
		delete [] temp->m_pcData;
		delete temp;
	}
	delete [] m_pBuffer->m_pcData;

#ifndef WIN32
pthread_mutex_destroy(&m_BufLock);
#else
CloseHandle(m_BufLock);
#endif
}


void CSndBuffer::addBuffer(const char* data, const int& len, const int& ttl, const bool& order)
{
//cout <<"ADD BUFFER!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
	int size = len / m_iMSS;
	if ((len % m_iMSS) != 0)
		size ++;

	// dynamically increase sender buffer
	while (size + m_iCount >= m_iSize)
		increase();

	uint64_t time = CTimer::getTime();
	int32_t inorder = order;
	inorder <<= 29;

	Block* s = m_pLastBlock;
	for (int i = 0; i < size; ++ i)
	{
		int pktlen = len - i * m_iMSS;
		if (pktlen > m_iMSS)
			pktlen = m_iMSS;

		memcpy(s->m_pcData, data + i * m_iMSS, pktlen);
		s->m_iLength = pktlen;

		s->m_iMsgNo = m_iNextMsgNo | inorder;
		if (i == 0)
			s->m_iMsgNo |= 0x80000000;
		if (i == size - 1)
			s->m_iMsgNo |= 0x40000000;

		s->m_OriginTime = time;
		s->m_iTTL = ttl;

		s = s->m_pNext;
		m_iNextMsgNo ++;
	}
	m_pLastBlock = s;

	CGuard::enterCS(m_BufLock);
	m_iCount += size;
	CGuard::leaveCS(m_BufLock);

/*	m_iNextMsgNo ++;
	if (m_iNextMsgNo == CMsgNo::m_iMaxMsgNo)
		m_iNextMsgNo = 1;*/
}

int CSndBuffer::addBufferFromFile(fstream& ifs, const int& len)
{
	int size = len / m_iMSS;
	if ((len % m_iMSS) != 0)
		size ++;
	// dynamically increase sender buffer
	while (size + m_iCount >= m_iSize)
		increase();

//cout << "TEST " << size << " " << m_iCount << " " << m_iSize << endl;

	Block* s = m_pLastBlock;
	int total = 0;
	for (int i = 0; i < size; ++ i)
	{
		if (ifs.bad() || ifs.fail() || ifs.eof())
			break;

		int pktlen = len - i * m_iMSS;
		if (pktlen > m_iMSS)
			pktlen = m_iMSS;

		ifs.read(s->m_pcData, pktlen);
		if ((pktlen = ifs.gcount()) <= 0)
			break;

		// currently file transfer is only available in streaming mode, message is always in order, ttl = infinite
/*		if (i == 0)
			s->m_iMsgNo |= 0x80000000;
		if (i == size - 1)
			s->m_iMsgNo |= 0x40000000;*/

		s->m_iLength = pktlen;
		s->m_iTTL = -1;
		s = s->m_pNext;

		total += pktlen;
	}
	m_pLastBlock = s;

/*cout << "check\n";
Block* tmp = FirstBuffer->first_block;
while (tmp != m_pLastBlock->m_pNext) {
	cout << tmp->m_iBufferNo << endl;
	tmp = tmp->m_pNext;
}
cout << "buffer\n";
Buffer* buffer_p = FirstBuffer;
while (buffer_p != bufferEnder) {
	cout << buffer_p->first_block->m_iBufferNo << endl;
	buffer_p = buffer_p->m_pNext;
}
cout << buffer_p->first_block->m_iBufferNo << endl;
cout << "end check\n";*/

	CGuard::enterCS(m_BufLock);
	m_iCount += size;
	CGuard::leaveCS(m_BufLock);

/*	m_iNextMsgNo ++;
	if (m_iNextMsgNo == CMsgNo::m_iMaxMsgNo)
		m_iNextMsgNo = 1;*/

	return total;
}

int CSndBuffer::readData(char** data, int32_t& msgno)
{
	// No data to read
	if (m_pCurrBlock == m_pLastBlock)
		return 0;

	*data = m_pCurrBlock->m_pcData;
	int readlen = m_pCurrBlock->m_iLength;
	msgno = m_pCurrBlock->m_iMsgNo;

	m_pCurrBlock = m_pCurrBlock->m_pNext;

	return readlen;
}

int CSndBuffer::readData(char** data, const int offset, int32_t& msgno, int& msglen)
{
	CGuard bufferguard(m_BufLock);

	// find the block
	int movement = (FirstBlock+offset)/m_pBuffer->m_iSize;
	int blockOffset = (FirstBlock+offset)%m_pBuffer->m_iSize;
	//cerr<<FirstBlock<<" "<<movement<<" "<<blockOffset<<" "<<bufferEnder->m_iSize<<" "<<offset<<endl;
	Buffer* buffer_p = FirstBuffer;
	for (int i=0; i<movement; ++i)
	{
		buffer_p = buffer_p->m_pNext;
	}
	Block* p;
	if (movement)
		p = buffer_p->first_block;
	else {
		p = m_pFirstBlock;
		blockOffset = offset;
	}
	for (int i=0; i<blockOffset; ++i)
	{
		p = p->m_pNext;
	}
	//cerr<<(CTimer::getTime() - p->m_OriginTime) / 1000<<" "<<((uint64_t)p->m_iTTL)<<endl;

/*	Block* test_p = m_pFirstBlock;
	for (int i = 0; i < offset; ++ i)
		test_p = test_p->m_pNext;
	if (test_p == p)
		cout << "correct read\n";
	else cout << "ERROR READING!!!!!\n";*/

	/*  Block* test_p = m_pFirstBlock;
   for (int i = 0; i < offset; ++ i) {
      test_p = test_p->m_pNext;
  //    if (test_p==p)
//	cerr<<"R "<<i+1<<endl;
//      if (test_p==buffer_p->first_block)
//	cerr<<"L "<<i+1<<endl;
   }
   Block* tmp = test_p;
   for (int i=offset; i<offset+10;++i)
   {
      tmp = tmp->m_pNext;
      if (tmp==p)
        cerr<<"R "<<i+1<<endl;
      if (tmp==buffer_p->first_block)
        cerr<<"L "<<i+1<<endl;
   }

cerr<<(CTimer::getTime() - test_p->m_OriginTime) / 1000<<" "<<((uint64_t)test_p->m_iTTL)<<endl;

   if (test_p!=p)
	cerr<<"WRONG\n";
	 */
	//   for (int i = 0; i < offset; ++ i)
	//      p = p->m_pNext;

	if ((p->m_iTTL >= 0) && ((CTimer::getTime() - p->m_OriginTime) / 1000 > (uint64_t)p->m_iTTL))
	{
		//cerr<<"DROP!!!"<<endl;
		msgno = p->m_iMsgNo & 0x1FFFFFFF;

		msglen = 1;
		p = p->m_pNext;
		bool move = false;
		while (msgno == (p->m_iMsgNo & 0x1FFFFFFF))
		{
			if (p == m_pCurrBlock)
				move = true;
			p = p->m_pNext;
			if (move)
				m_pCurrBlock = p;
			msglen ++;
		}

		return -1;
	}

	*data = p->m_pcData;
	int readlen = p->m_iLength;
	msgno = p->m_iMsgNo;

	return readlen;
}

void CSndBuffer::ackData(const int& offset)
{
	CGuard bufferguard(m_BufLock);

/*	Block* tmp = m_pFirstBlock, *tmp1 = m_pFirstBlock;
	for (int i = 0; i < offset; ++ i)
		tmp = tmp->m_pNext;*/


	//   for (int i = 0; i < offset; ++ i)
	//      m_pFirstBlock = m_pFirstBlock->m_pNext;
	int movement = (FirstBlock+offset)/m_pBuffer->m_iSize;
//	cout << "first buffer " << FirstBuffer << " " << FirstBuffer->first_block << endl;
	for (int i=0; i<movement; ++i) {
		FirstBuffer = FirstBuffer->m_pNext;
//		cout << "first buffer " << FirstBuffer << " " << FirstBuffer->first_block << endl;
	}
	FirstBlock = (FirstBlock+offset)%m_pBuffer->m_iSize;

	int blockOffset;
	if (movement) {
		blockOffset = FirstBlock;
		m_pFirstBlock = FirstBuffer->first_block;
//		cout << "testtt " << FirstBuffer->first_block->m_iBufferNo << endl;
	} else {
		blockOffset = offset;
	}

	for (int i=0; i<blockOffset; ++i)
	{
		m_pFirstBlock = m_pFirstBlock->m_pNext;
	}

/*if (m_pFirstBlock == tmp)
	cout << "correct ack\n";
else {
	cout << "error in ackData\n";
	cout << "test " << test << " " << offset << " " << movement << " " << blockOffset << " " << m_pBuffer->m_iSize-1 << " " << FirstBlock << " " << m_pFirstBlock->m_iBufferNo << " " << tmp->m_iBufferNo << endl;
	cout << tmp1 << endl;
	for (int i = 0; i < offset; ++i) {
		tmp1 = tmp1->m_pNext;
		if (tmp1 == FirstBuffer-> first_block)
			cout << "Here\n";
		cout << tmp1 -> m_iBufferNo << " " << tmp1 << endl;
	}
}*/

	m_iCount -= offset;

	CTimer::triggerEvent();
}

int CSndBuffer::getCurrBufSize() const
{
	return m_iCount;
}

void CSndBuffer::increase()
{
        CGuard::enterCS(m_BufLock);

        int unitsize = m_pBuffer->m_iSize;
	// new physical buffer
	Buffer* nbuf = NULL;
	try
	{
		nbuf  = new Buffer;
		nbuf->m_pcData = new char [unitsize * m_iMSS];
	}
	catch (...)
	{
		delete nbuf;
		throw CUDTException(3, 2, 0);
	}
	nbuf->m_iSize = unitsize;
	while (bufferEnder->m_pNext != FirstBuffer)
		bufferEnder = bufferEnder->m_pNext;
//	cout << "increase size at " << bufferEnder->first_block->m_iBufferNo << endl;

	Block* prev_first_block;
	if (m_pFirstBlock->m_iBufferNo) {
//		cout << "firstblock " << m_pFirstBlock->m_iBufferNo << " " << m_pFirstBlock << endl;
		prev_first_block = FirstBuffer->first_block;
//		cout << "prev " << prev_first_block->m_iBufferNo << " " << prev_first_block << endl;
		while (prev_first_block->m_pNext != m_pFirstBlock) {
			prev_first_block = prev_first_block->m_pNext;
//			cout << prev_first_block->m_iBufferNo << " " << prev_first_block << endl;
		}
	} else {
//		cout << "firstblock " << m_pFirstBlock->m_iBufferNo << " " << m_pFirstBlock << endl;
		prev_first_block = bufferEnder->first_block;
//		cout << "prev " << prev_first_block->m_iBufferNo << " " << prev_first_block << endl;
		while (prev_first_block->m_pNext != m_pFirstBlock) {
			prev_first_block = prev_first_block->m_pNext;
//			cout << prev_first_block->m_iBufferNo << " " << prev_first_block << endl;
		}
	}

	// insert the buffer at the end of the buffer list
	nbuf->m_pNext = bufferEnder->m_pNext;
	bufferEnder->m_pNext = nbuf;
	bufferEnder = nbuf;

/*cout << "increase\n";
int counter = 0;
Buffer* buffer_p = FirstBuffer;
while (buffer_p != bufferEnder) {
	cout << counter << endl;
	buffer_p = buffer_p->m_pNext;
	counter++;
}
cout << "end increase\n";*/

	// new packet blocks
	Block* nblk = NULL;
	try
	{
		nblk = new Block;
	}
	catch (...)
	{
		delete nblk;
		throw CUDTException(3, 2, 0);
	}
	Block* pb = nblk, *first_block;
	int bufferNo = m_pFirstBlock->m_iBufferNo;
//	cout << "test " << bufferNo << endl;
//HACK: This is a temp solution, could it break file transfer?
//first_block = NULL;
	for (int i = 1; i < unitsize; ++ i)
	{
		pb->m_pNext = new Block;
		pb->m_iBufferNo = bufferNo;
		if (!bufferNo) {
			first_block = pb;
		}
		pb = pb->m_pNext;
		bufferNo = (bufferNo+1)%unitsize;
	}

	// insert the new blocks onto the existing one
	pb->m_iBufferNo = bufferNo;
	if (!bufferNo) {
		first_block = pb;
	}
	pb->m_pNext = prev_first_block->m_pNext;
   	prev_first_block->m_pNext = nblk;
//   	cout << prev_first_block << " " << m_pFirstBlock << " " << pb << " " << nblk << endl;

	if (m_pFirstBlock->m_iBufferNo) {
		bufferEnder->first_block = FirstBuffer->first_block;
		FirstBuffer->first_block = first_block;
//		cout << "testtttt increase " <<  bufferEnder->first_block->m_iBufferNo << " " << FirstBuffer->first_block->m_iBufferNo << endl;
	} else {
		bufferEnder->first_block = first_block;
//		cout << "testtttt increase " << bufferEnder->first_block->m_iBufferNo << endl;
	}

/*cout << "increase testing" << endl;
Block *tmp = bufferEnder->first_block;
while (tmp != m_pFirstBlock) {
	cout << tmp->m_iBufferNo << " " << tmp << endl;
	tmp = tmp->m_pNext;
}
cout << "end increase testing\n";*/


	// insert the new blocks onto the existing one
	pb = nblk;
	char* pc = nbuf->m_pcData;
	for (int i = 0; i < unitsize; ++ i)
	{
		pb->m_pcData = pc;
		pb = pb->m_pNext;
		pc += m_iMSS;
	}

        CGuard::leaveCS(m_BufLock);
	/*   Block* test_p = m_pFirstBlock;
   for (int i = 0; i < m_iSize+33; ++ i) {
      test_p = test_p->m_pNext;
      if (test_p==bufferEnder->first_block)
        cerr<<"AA "<<i+1<<endl;
   }
	 */

	m_iSize += unitsize;
	//cout<<"Increase!"<<endl;
//	cerr<<"Increase!"<<endl;
	//cout<<m_iSize<<endl;
}

////////////////////////////////////////////////////////////////////////////////

CRcvBuffer::CRcvBuffer(CUnitQueue* queue, const int& bufsize):
		m_pUnit(NULL),
		m_iSize(bufsize),
		m_pUnitQueue(queue),
		m_iStartPos(0),
		m_iLastAckPos(0),
		m_iMaxPos(0),
		m_iNotch(0)
{
	m_pUnit = new CUnit* [m_iSize];
	for (int i = 0; i < m_iSize; ++ i)
		m_pUnit[i] = NULL;
}

CRcvBuffer::~CRcvBuffer()
{
	for (int i = 0; i < m_iSize; ++ i)
	{
		if (NULL != m_pUnit[i])
		{
			m_pUnit[i]->m_iFlag = 0;
			-- m_pUnitQueue->m_iCount;
		}
	}

	delete [] m_pUnit;
}

int CRcvBuffer::addData(CUnit* unit, int offset)
{
	int pos = (m_iLastAckPos + offset) % m_iSize;
	if (offset > m_iMaxPos)
		m_iMaxPos = offset;

	if (NULL != m_pUnit[pos])
		return -1;

	m_pUnit[pos] = unit;

	unit->m_iFlag = 1;
	++ m_pUnitQueue->m_iCount;

	return 0;
}

int CRcvBuffer::readBuffer(char* data, const int& len)
{
	int p = m_iStartPos;
	int lastack = m_iLastAckPos;
	int rs = len;

	while ((p != lastack) && (rs > 0))
	{
		int unitsize = m_pUnit[p]->m_Packet.getLength() - m_iNotch;
		if (unitsize > rs)
			unitsize = rs;

		memcpy(data, m_pUnit[p]->m_Packet.m_pcData + m_iNotch, unitsize);
		data += unitsize;

		if ((rs > unitsize) || (rs == m_pUnit[p]->m_Packet.getLength() - m_iNotch))
		{
			CUnit* tmp = m_pUnit[p];
			m_pUnit[p] = NULL;
			tmp->m_iFlag = 0;
			-- m_pUnitQueue->m_iCount;

			if (++ p == m_iSize)
				p = 0;

			m_iNotch = 0;
		}
		else
			m_iNotch += rs;

		rs -= unitsize;
	}

	m_iStartPos = p;
	return len - rs;
}

int CRcvBuffer::readBufferToFile(fstream& ofs, const int& len)
{
	int p = m_iStartPos;
	int lastack = m_iLastAckPos;
	int rs = len;

	while ((p != lastack) && (rs > 0))
	{
		int unitsize = m_pUnit[p]->m_Packet.getLength() - m_iNotch;
		if (unitsize > rs)
			unitsize = rs;

		ofs.write(m_pUnit[p]->m_Packet.m_pcData + m_iNotch, unitsize);
		if (ofs.fail())
			break;

		if ((rs > unitsize) || (rs == m_pUnit[p]->m_Packet.getLength() - m_iNotch))
		{
			CUnit* tmp = m_pUnit[p];
			m_pUnit[p] = NULL;
			tmp->m_iFlag = 0;
			-- m_pUnitQueue->m_iCount;

			if (++ p == m_iSize)
				p = 0;

			m_iNotch = 0;
		}
		else
			m_iNotch += rs;

		rs -= unitsize;
	}

	m_iStartPos = p;

	return len - rs;
}

void CRcvBuffer::ackData(const int& len)
{
	m_iLastAckPos = (m_iLastAckPos + len) % m_iSize;
	m_iMaxPos -= len;
	if (m_iMaxPos < 0)
		m_iMaxPos = 0;

	CTimer::triggerEvent();
}

int CRcvBuffer::getAvailBufSize() const
{
	// One slot must be empty in order to tell the difference between "empty buffer" and "full buffer"
	return m_iSize - getRcvDataSize() - 1;
}

int CRcvBuffer::getRcvDataSize() const
{
	if (m_iLastAckPos >= m_iStartPos)
		return m_iLastAckPos - m_iStartPos;

	return m_iSize + m_iLastAckPos - m_iStartPos;
}

void CRcvBuffer::dropMsg(const int32_t& msgno)
{
	for (int i = m_iStartPos, n = (m_iLastAckPos + m_iMaxPos) % m_iSize; i != n; i = (i + 1) % m_iSize)
		if ((NULL != m_pUnit[i]) && (msgno == m_pUnit[i]->m_Packet.m_iMsgNo))
			m_pUnit[i]->m_iFlag = 3;
}

int CRcvBuffer::readMsg(char* data, const int& len)
{
	int p, q;
	bool passack;
	if (!scanMsg(p, q, passack))
		return 0;

	int rs = len;
	while (p != (q + 1) % m_iSize)
	{
		int unitsize = m_pUnit[p]->m_Packet.getLength();
		if ((rs >= 0) && (unitsize > rs))
			unitsize = rs;

		if (unitsize > 0)
		{
			memcpy(data, m_pUnit[p]->m_Packet.m_pcData, unitsize);
			data += unitsize;
			rs -= unitsize;
		}

		if (!passack)
		{
			CUnit* tmp = m_pUnit[p];
			m_pUnit[p] = NULL;
			tmp->m_iFlag = 0;
			-- m_pUnitQueue->m_iCount;
		}
		else
			m_pUnit[p]->m_iFlag = 2;

		if (++ p == m_iSize)
			p = 0;
	}

	if (!passack)
		m_iStartPos = (q + 1) % m_iSize;

	return len - rs;
}

int CRcvBuffer::getRcvMsgNum()
{
	int p, q;
	bool passack;
	return scanMsg(p, q, passack) ? 1 : 0;
}

bool CRcvBuffer::scanMsg(int& p, int& q, bool& passack)
{
	// empty buffer
	if ((m_iStartPos == m_iLastAckPos) && (m_iMaxPos <= 0))
		return false;

	//skip all bad msgs at the beginning
	while (m_iStartPos != m_iLastAckPos)
	{
		if (NULL == m_pUnit[m_iStartPos])
		{
			if (++ m_iStartPos == m_iSize)
				m_iStartPos = 0;
			continue;
		}

		if ((1 == m_pUnit[m_iStartPos]->m_iFlag) && (m_pUnit[m_iStartPos]->m_Packet.getMsgBoundary() > 1))
		{
			bool good = true;

			// look ahead for the whole message
			for (int i = m_iStartPos; i != m_iLastAckPos;)
			{
				if ((NULL == m_pUnit[i]) || (1 != m_pUnit[i]->m_iFlag))
				{
					good = false;
					break;
				}

				if ((m_pUnit[i]->m_Packet.getMsgBoundary() == 1) || (m_pUnit[i]->m_Packet.getMsgBoundary() == 3))
					break;

				if (++ i == m_iSize)
					i = 0;
			}

			if (good)
				break;
		}

		CUnit* tmp = m_pUnit[m_iStartPos];
		m_pUnit[m_iStartPos] = NULL;
		tmp->m_iFlag = 0;
		-- m_pUnitQueue->m_iCount;

		if (++ m_iStartPos == m_iSize)
			m_iStartPos = 0;
	}

	p = -1;                  // message head
	q = m_iStartPos;         // message tail
	passack = m_iStartPos == m_iLastAckPos;
	bool found = false;

	// looking for the first message
	for (int i = 0, n = m_iMaxPos + getRcvDataSize(); i <= n; ++ i)
	{
		if ((NULL != m_pUnit[q]) && (1 == m_pUnit[q]->m_iFlag))
		{
			switch (m_pUnit[q]->m_Packet.getMsgBoundary())
			{
			case 3: // 11
			p = q;
			found = true;
			break;

			case 2: // 10
				p = q;
				break;

			case 1: // 01
				if (p != -1)
					found = true;
			}
		}
		else
		{
			// a hole in this message, not valid, restart search
			p = -1;
		}

		if (found)
		{
			// the msg has to be ack'ed or it is allowed to read out of order, and was not read before
			if (!passack || !m_pUnit[q]->m_Packet.getMsgOrderFlag())
				break;

			found = false;
		}

		if (++ q == m_iSize)
			q = 0;

		if (q == m_iLastAckPos)
			passack = true;
	}

	// no msg found
	if (!found)
	{
		// if the message is larger than the receiver buffer, return part of the message
		if ((p != -1) && ((q + 1) % m_iSize == p))
			found = true;
	}

	return found;
}
