/*
http://ac.els-cdn.com/S1389128615002285/1-s2.0-S1389128615002285-main.pdf?_tid=eae6df82-6123-11e6-af66-00000aab0f6b&acdnat=1471071937_f601bbe4a40a2f5adb17c86d412dc564
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <random>
#include "routing_wsnrandomcoder.h"
#include "network_ip.h"
#include "api.h"
#include "advertising_wsn_neighborinfo_diffusion.h"
#include "partition.h"
#include "loggeneration.h"

#include "routing_wsnflooding.h"
#include "routing_wsnforwarding.h"
#include "mapping.h"

#define RANDOM_POOL 100000
#define PROB_CODER  0.5
#define ROLE_SWITCH_TIME 10*SECOND

bool operator<(const NativePacketPair& lhs, const NativePacketPair& rhs)
{
	return lhs.first < rhs.first;
}
bool operator>(const NativePacketPair& lhs, const NativePacketPair& rhs)
{
	return lhs.first > rhs.first;
}
bool operator==(const NativePacketPair& lhs, const NativePacketPair& rhs)
{
	return lhs.first == rhs.first;
}

bool operator<(const CodedPacketInfo& lhs, const CodedPacketInfo& rhs)
{
	if (lhs.leftNode < rhs.leftNode)
		return true;
	if (lhs.leftNode > rhs.leftNode)
		return false;
	if (lhs.rightNode < rhs.rightNode)
		return true;
	if (lhs.rightNode > rhs.rightNode)
		return false;
	if (lhs.leftPacketId < rhs.leftPacketId)
		return true;
	if (lhs.leftPacketId > rhs.leftPacketId)
		return false;
	if (lhs.rightPacketId < rhs.rightPacketId)
		return true;
	if (lhs.rightPacketId > rhs.rightPacketId)
		return false;
	return false;
}
bool operator>(const CodedPacketInfo& lhs, const CodedPacketInfo& rhs)
{
	if (lhs.leftNode > rhs.leftNode)
		return true;
	if (lhs.leftNode < rhs.leftNode)
		return false;
	if (lhs.rightNode > rhs.rightNode)
		return true;
	if (lhs.rightNode < rhs.rightNode)
		return false;
	if (lhs.leftPacketId > rhs.leftPacketId)
		return true;
	if (lhs.leftPacketId < rhs.leftPacketId)
		return false;
	if (lhs.rightPacketId > rhs.rightPacketId)
		return true;
	if (lhs.rightPacketId < rhs.rightPacketId)
		return false;
	return false;
}

bool operator==(const CodedPacketInfo& lhs, const CodedPacketInfo& rhs)
{
	return lhs.leftNode==rhs.leftNode && lhs.leftPacketId==rhs.leftPacketId
		&&lhs.rightNode==rhs.rightNode && lhs.rightPacketId==rhs.rightPacketId;
}

bool operator<(const IntermediatePacketPair& lhs, const IntermediatePacketPair& rhs)
{
	if(lhs.first < rhs.first)
		return true;
	else if(lhs.first > rhs.first)
		return false;
	else if(lhs.second < rhs.second)
		return true;
	else 
		return false;
}
bool operator>(const IntermediatePacketPair& lhs, const IntermediatePacketPair& rhs)
{
	if(lhs.first > rhs.first)
		return true;
	else if(lhs.first < rhs.first)
		return false;
	else if(lhs.second > rhs.second)
		return true;
	else 
		return false;
}
bool operator==(const IntermediatePacketPair& lhs, const IntermediatePacketPair& rhs)
{
	return (rhs.first==lhs.first && rhs.second == lhs.second);
}


void WsnRandomCoderInit(Node* node, 
						const NodeInput* nodeInput,
						int interfaceIndex)
{
	NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
	NetworkIpSetRouterFunction(node,
							   &WsnRandomCoderRouterFunction,
							   DEFAULT_INTERFACE);
	
	WsnRandomCoderData* wsnRandomCoderData=new WsnRandomCoderData;
	ip->wsnRandomCoderData = wsnRandomCoderData;
	wsnRandomCoderData->nativePacketDB = new NativePacketDB;
	wsnRandomCoderData->codedPacketDB = new CodedPacketDB;
	wsnRandomCoderData->coder = false;
	wsnRandomCoderData->intermediatePacketPair = NULL;
	wsnRandomCoderData->waitingPacket = NULL;
	double coderProb;
	BOOL wasFound;
	IO_ReadDouble(node,
				  node->nodeId,
				  DEFAULT_INTERFACE, 
				  nodeInput, 
				  "CODER_PROB",
				  &wasFound,
				  &coderProb);
	ERROR_Assert(wasFound, "CODER_PROB header is not found in input .config file");
	wsnRandomCoderData->coderProb = coderProb;	
	WsnRandomCoderRoleSelection(node,NULL);
}

void WsnRandomCoderHandleProtocolEvent(Node* node, Message* msg)
{
	switch(msg->eventType)
	{
		case MSG_ROUTING_WsnRoleChangeTimerExpired:
			{
				WsnRandomCoderRoleSelection(node, msg);
				break;
			}
		default:
			{
				break;
			}

		
	}
	MESSAGE_Free(node, msg);
}

void WsnRandomCoderRoleSelection(Node* node, Message* msg)
{
	NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
	NeighborDB* neighborDB  = (NeighborDB*)ip->neighborDB;
	WsnRandomCoderData* wsnRandomCoderData= (WsnRandomCoderData*)ip->wsnRandomCoderData;
	bool wasCoder = wsnRandomCoderData->coder;

	srand (time(NULL));
	int randInt = rand() % RANDOM_POOL;

	std::mt19937 rng;
	std::uniform_int_distribution<std::mt19937::result_type> dist6(0,RANDOM_POOL);
	rng.seed(std::random_device()());
	randInt =dist6(rng);
	
	

	if (neighborDB->myHopcount==1)
	{
		if(randInt < (wsnRandomCoderData->coderProb*RANDOM_POOL))
			wsnRandomCoderData->coder=true;
		else
			wsnRandomCoderData->coder=false;
	}
	else
		wsnRandomCoderData->coder=false;

	if(wasCoder && !wsnRandomCoderData->coder && wsnRandomCoderData->intermediatePacketPair != NULL)
	{
		BOOL packetWasRouted;
		IpHeaderType* ipHeader = (IpHeaderType*) wsnRandomCoderData->waitingPacket->packet;
		WsnForwardingRouterFunction(node,wsnRandomCoderData->waitingPacket,ipHeader->ip_dst,0,&packetWasRouted);
		delete wsnRandomCoderData->intermediatePacketPair;
		wsnRandomCoderData->intermediatePacketPair = NULL;
		wsnRandomCoderData->waitingPacket = NULL;
	}		
	Message* newMsg = (Message*) MESSAGE_Alloc(node,
								NETWORK_LAYER,
								ROUTING_PROTOCOL_WSN_RANDOM_CODER,
								MSG_ROUTING_WsnRoleChangeTimerExpired);
	MESSAGE_Send(node, newMsg, ROLE_SWITCH_TIME);
}

void WsnRandomCoderRouterFunction(Node* node,
							Message* msg, 
							NodeAddress destAddr,
							NodeAddress previousHopAddress,
							BOOL* packetWasRouted)
{
	NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
	WsnRandomCoderData* wsnRandomCoderData= (WsnRandomCoderData*)ip->wsnRandomCoderData;
	IpHeaderType* ipHeader = (IpHeaderType *) msg->packet;
	NodeAddress myIpAddress = NetworkIpGetInterfaceAddress(node,DEFAULT_INTERFACE);
	NeighborDB* neighborDB  = (NeighborDB*)ip->neighborDB;

	/*if(wsnRandomCoderData->packetSample == NULL)
	{
		wsnRandomCoderData->packetSample = MESSAGE_Duplicate(node,msg);
	}*/

	if(myIpAddress==ipHeader->ip_dst)
	{
		*packetWasRouted=FALSE;
		return;
	}

	bool iAmSender = (myIpAddress == ipHeader->ip_src)
		&&(ipHeader->prevHopcount==0);

	bool fromUpstream = ipHeader->prevHopcount == neighborDB->myHopcount+1;

	if(!iAmSender && !fromUpstream)
	{
		MESSAGE_Free(node,msg);
		*packetWasRouted=TRUE;
		return;
	}
	
	if(iAmSender){
		Message* duplicateMsg = (Message*)MESSAGE_Duplicate(node,msg);
		MESSAGE_SetLayer(duplicateMsg,MAC_LAYER,0);
		MESSAGE_SetEvent(duplicateMsg,MSG_MAC_FromNetwork);
		NativePacketPair nativePacketPair;
		nativePacketPair.first=ipHeader->ip_id;
		nativePacketPair.second = duplicateMsg;

		//ipHeader->codedPacket=0;


		wsnRandomCoderData->nativePacketDB->insert(nativePacketPair);
		WsnForwardingRouterFunction(node,
									msg,
									destAddr,
									previousHopAddress,
									packetWasRouted);
	}

	else if(neighborDB->myHopcount==2)
	{
		srand(time(NULL));
		clocktype delay = (rand()%500)*MILLI_SECOND;
		ipHeader->prevHopcount=neighborDB->myHopcount;
		*packetWasRouted=TRUE;
		NetworkIpSendPacketToMacLayerWithDelay(node,
											  msg,
											  DEFAULT_INTERFACE,
											  ANY_DEST,
											  delay);
		PrintDataMessageTransmissionEvent(node);
	}

	else if(neighborDB->myHopcount==1)
	{
		if(wsnRandomCoderData->coder==false)
		{
			//ipHeader->codedPacket=0;
			WsnForwardingRouterFunction(node,
										msg,
										destAddr,
										previousHopAddress,
										packetWasRouted);
		}
		else if(wsnRandomCoderData->coder==true)
		{
			if(wsnRandomCoderData->intermediatePacketPair == NULL)
			{
				wsnRandomCoderData->intermediatePacketPair 
					= new IntermediatePacketPair;
				wsnRandomCoderData->intermediatePacketPair->first 
					                            = ipHeader->ip_src;
				wsnRandomCoderData->intermediatePacketPair->second 
					                            = ipHeader->ip_id;
				//MESSAGE_Free(node,msg);
				wsnRandomCoderData->waitingPacket = msg;
				*packetWasRouted=TRUE;
				return;
			}
			else
			{
				if(wsnRandomCoderData->intermediatePacketPair->first == ipHeader->ip_src
					&& wsnRandomCoderData->intermediatePacketPair->second == ipHeader->ip_id)
				{
					MESSAGE_Free(node,msg);
					*packetWasRouted = TRUE;
					return;
				}		
				//ipHeader->codedPacket=1;
				IpOptionsHeaderType* ipOption;
				XorCodingOption xorCodingOption;
				xorCodingOption.anotherAddr = wsnRandomCoderData->intermediatePacketPair->first;
				xorCodingOption.anotherId = wsnRandomCoderData->intermediatePacketPair->second;
				AddIpOptionField(node,msg,IPOPT_XOR_CODING_HEADER,sizeof(XorCodingOption));
				ipOption = FindAnIpOptionField((IpHeaderType*)msg->packet,IPOPT_XOR_CODING_HEADER);
				memcpy((char*)ipOption+sizeof(IpOptionsHeaderType),(char*)&xorCodingOption,sizeof(XorCodingOption));
				WsnForwardingRouterFunction(node,msg,destAddr,
											previousHopAddress,
											packetWasRouted);
				/*ipHeader->leftNodeAddr = ipHeader->ip_src;
				ipHeader->leftId = ipHeader->ip_id;
				ipHeader->rightNodeAddr 
				= wsnRandomCoderData->intermediatePacketPair->first;
				ipHeader->rightId
				= wsnRandomCoderData->intermediatePacketPair->second;*/
				delete wsnRandomCoderData->intermediatePacketPair;
				wsnRandomCoderData->intermediatePacketPair=NULL;
				MESSAGE_Free(node,wsnRandomCoderData->waitingPacket);
				wsnRandomCoderData->waitingPacket=NULL;
				*packetWasRouted =TRUE;
				return;
			}
		}
	}
}

void WsnRandomCoderDecoding(Node* node,
							Message* msg)
{
	
	NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
	WsnRandomCoderData* wsnRandomCoderData= (WsnRandomCoderData*)ip->wsnRandomCoderData;
	Node* sourceNode;
	Node* leftNode;
	Node* rightNode;
	Node* anotherNode;
	Message* anotherMsg;
	int anotherId;
	bool found;
	IpHeaderType* ipHeader = (IpHeaderType*) msg->packet;

	NodeAddress sourceNodeId 
		= MAPPING_GetNodeIdFromInterfaceAddress((node->partitionData->firstNode),
										  ipHeader->ip_src);
	sourceNode=MAPPING_GetNodePtrFromHash(node->partitionData->nodeIdHash,
										  sourceNodeId);	
	NetworkDataIp* sourceIp = (NetworkDataIp*) sourceNode->networkData.networkVar;
	WsnRandomCoderData* wsnSourceRandomCoderData= (WsnRandomCoderData*)sourceIp->wsnRandomCoderData;
	NativePacketDB::iterator spktIt;
	NetworkDataIp* anotherIp;
	WsnRandomCoderData* wsnAnotherRandomCoderData;
	CodedPacketDB::iterator it;
	NativePacketPair temp;

	bool codedPacket= (IpHeaderSize(ipHeader) != sizeof(IpHeaderType));

	NodeAddress sourceAddress, destinationAddress;
	TosType priority;
	unsigned char protocol;
	unsigned ttl;

	if(!codedPacket)
	{
		temp.first=ipHeader->ip_id;
		temp.second=NULL;
		spktIt=wsnSourceRandomCoderData->nativePacketDB->find(temp);
		if(spktIt!=wsnSourceRandomCoderData->nativePacketDB->end())
		{
			
			NetworkIpRemoveIpHeader(node,spktIt->second,&sourceAddress,&destinationAddress,&priority,&protocol, &ttl);
			SendToUdp(node, spktIt->second, priority, sourceAddress, destinationAddress,
				DEFAULT_INTERFACE);
			wsnSourceRandomCoderData->nativePacketDB->erase(spktIt);
		}
		MESSAGE_Free(node,msg);  
		unsigned char ipProtocolNumber;
		unsigned ttl=0;

		found=false;
		for(it=wsnRandomCoderData->codedPacketDB->begin();
			it!=wsnRandomCoderData->codedPacketDB->end();
			it++)
		{
			if(it->leftNode==sourceNode && it->leftPacketId == ipHeader->ip_id)
			{
				
				anotherNode=it->rightNode;
				anotherId = it->rightPacketId;
				CodedPacketDB tempDB;
				CodedPacketInfo tempInfo=*it;
				tempDB = *(wsnRandomCoderData->codedPacketDB);
				tempDB.erase(*it);
				wsnRandomCoderData->codedPacketDB->clear();
				*(wsnRandomCoderData->codedPacketDB) = tempDB;
				it=wsnRandomCoderData->codedPacketDB->find(tempInfo);
				found=true;
			}
			else if (it->rightNode==sourceNode && it->rightPacketId == ipHeader->ip_id)
			{
				anotherNode=it->leftNode;
				anotherId = it->leftPacketId;
				CodedPacketDB tempDB;
				CodedPacketInfo tempInfo=*it;
				tempDB = *(wsnRandomCoderData->codedPacketDB);
				tempDB.erase(*it);
				wsnRandomCoderData->codedPacketDB->clear();
				*(wsnRandomCoderData->codedPacketDB) = tempDB;
				it=wsnRandomCoderData->codedPacketDB->find(tempInfo);
				found=true;
			}
			if(found)
			{

				anotherIp = (NetworkDataIp*) anotherNode->networkData.networkVar;
				wsnAnotherRandomCoderData= (WsnRandomCoderData*)anotherIp->wsnRandomCoderData;
				for(spktIt = wsnAnotherRandomCoderData->nativePacketDB->begin();
					spktIt != wsnAnotherRandomCoderData->nativePacketDB->end();
					spktIt++)
				{
					if (spktIt->first==anotherId)
					{
						Message* bonusPacket = MESSAGE_Duplicate(node,spktIt->second);
						/*NetworkIpRemoveIpHeader(node,
												spktIt->second,
												&sourceAddress,
												&destinationAddress,
												&priority,
												&ipProtocolNumber,
												&ttl);
						SendToUdp(node, spktIt->second, priority, sourceAddress, destinationAddress,
							DEFAULT_INTERFACE);*/

						//wsnAnotherRandomCoderData->nativePacketDB->erase(spktIt);
						WsnRandomCoderDecoding(node,bonusPacket);
						break;
					}
				}
			}

		}
		return;
		//search for another decodable packets
		//if find, remove it from codedPacketDB and look if it is freed or not
		//if not freed, send to UDP after removing ipHeader

	}
	else //coded packet
	{
		IpOptionsHeaderType* ipOption 
			= (IpOptionsHeaderType*) FindAnIpOptionField(ipHeader,IPOPT_XOR_CODING_HEADER);
		XorCodingOption xorCodingOption;
		memcpy((char*)&xorCodingOption,
			   (char*)ipOption+sizeof(IpOptionsHeaderType),
			   sizeof(XorCodingOption));

		bool leftPktFreed=true;
		bool rightPktFreed=true;

		NodeAddress leftNodeId
			= MAPPING_GetNodeIdFromInterfaceAddress(node->partitionData->firstNode,
													ipHeader->ip_src);
		NodeAddress rightNodeId
			=MAPPING_GetNodeIdFromInterfaceAddress(node->partitionData->firstNode,
													xorCodingOption.anotherAddr);

		Node* leftNode = MAPPING_GetNodePtrFromHash(
							node->partitionData->nodeIdHash,
							leftNodeId);
		Node* rightNode = MAPPING_GetNodePtrFromHash(
							node->partitionData->nodeIdHash,
							rightNodeId);

		NetworkDataIp* leftIp = (NetworkDataIp*) leftNode->networkData.networkVar;
		NetworkDataIp* rightIp= (NetworkDataIp*) rightNode->networkData.networkVar;

		WsnRandomCoderData* wsnLeftRandomCoderdata 
							= (WsnRandomCoderData*) leftIp->wsnRandomCoderData;
		WsnRandomCoderData* wsnRightRandomCoderdata 
							= (WsnRandomCoderData*) rightIp->wsnRandomCoderData;

		int leftId = ipHeader->ip_id;
		int rightId = xorCodingOption.anotherId;


		//examine whether right and left packet is decoded or not
		//if both of them is not decoded, put the packets in codedPacketDB and free the message
		NativePacketDB::iterator rpktIt;
		NativePacketDB::iterator lpktIt;
		

		temp.first = leftId;
		temp.second = NULL;

		lpktIt=wsnLeftRandomCoderdata->nativePacketDB->find(temp);
		if(lpktIt!=wsnLeftRandomCoderdata->nativePacketDB->end())
		{
			leftPktFreed=false;
		}

		temp.first = rightId;
		temp.second = NULL;

		rpktIt=wsnRightRandomCoderdata->nativePacketDB->find(temp);
		if(rpktIt!=wsnRightRandomCoderdata->nativePacketDB->end())
		{
			rightPktFreed=false;
		}
		unsigned char ipProtocolNumber;
		unsigned ttl=0;
		if(leftPktFreed==true && rightPktFreed==false)
		{
			Message* bonusPacket = MESSAGE_Duplicate(node, rpktIt->second);
			WsnRandomCoderDecoding(node,bonusPacket);
			/*NetworkIpRemoveIpHeader(node,
				rpktIt->second,
				&sourceAddress,
				&destinationAddress,
				&priority,
				&ipProtocolNumber,
				&ttl);
			SendToUdp(node, rpktIt->second, priority, sourceAddress, destinationAddress,
				DEFAULT_INTERFACE);
			wsnRightRandomCoderdata->nativePacketDB->erase(rpktIt);*/
			//send right packet to UDP
			//just free the msg
		}
		else if (leftPktFreed==false && rightPktFreed==true)
		{
			Message* bonusPacket = MESSAGE_Duplicate(node, lpktIt->second);
			WsnRandomCoderDecoding(node,bonusPacket);
			/*NetworkIpRemoveIpHeader(node,
									lpktIt->second,
									&sourceAddress,
									&destinationAddress,
									&priority,
									&ipProtocolNumber,
									&ttl);
			SendToUdp(node, lpktIt->second, priority, sourceAddress, destinationAddress,
						DEFAULT_INTERFACE);
			wsnLeftRandomCoderdata->nativePacketDB->erase(lpktIt);*/
			//send left packet to UDP
			//free the msg
		}

		else if (leftPktFreed==true && rightPktFreed==true)
		{

			//Do nothing
		}
		else
		{
			CodedPacketInfo codedPacketInfo;
			codedPacketInfo.leftNode = leftNode;
			codedPacketInfo.rightNode = rightNode;
			codedPacketInfo.leftPacketId = leftId;
			codedPacketInfo.rightPacketId = rightId;
			wsnRandomCoderData->codedPacketDB->insert(codedPacketInfo);
		}
		MESSAGE_Free(node,msg);
	}
	
}