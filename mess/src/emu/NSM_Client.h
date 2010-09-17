#include "NSM_Common.h"

#include "zlib.h"

Client *createGlobalClient();

void deleteGlobalClient();

#define MAX_COMPRESSED_OUTBUF_SIZE (1024*1024*64)

class Client
{
protected:
	RakNet::RakPeerInterface *rakInterface;

	vector<MemoryBlock> blocks,staleBlocks,xorBlocks;
    vector<unsigned char> incomingMsg;

    vector<MemoryBlock> constBlocks;

public:
	Client();

    ~Client();

	MemoryBlock createMemoryBlock(int size);

	MemoryBlock createMemoryBlock(unsigned char* ptr,int size);

	bool initializeConnection(const char *hostname,const char *port);

    std::pair<bool,bool> syncAndUpdate();

    void loadInitialData(unsigned char *data,int size);

    void resync(unsigned char *data,int size);

    void addConstData(unsigned char *data,int size);

	void checkMatch(Server *server);

    void sendString(const string &outputString);

    string getLatencyString();

    string getStatisticsString();

	MemoryBlock getMemoryBlock(int i)
	{
		return blocks[i];
	}

    int getNumConstBlocks()
    {
        return int(constBlocks.size());
    }

    MemoryBlock* getConstBlock(int i)
    {
        return &constBlocks[i];
    }

    void destroyConstBlock(int i)
    {
      constBlocks.erase(constBlocks.begin()+i);
    }
};
