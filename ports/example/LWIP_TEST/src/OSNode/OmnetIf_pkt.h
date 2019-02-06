

#include <iostream>
#include <sstream>

#ifndef _OMNETIF_PKT_H_ // _OMNETIF_PKT_H_
#define _OMNETIF_PKT_H_ //_OMNETIF_PKT_H_

static int seq_number = 0;
class OmnetIf_pkt
{

    

  public:

    int pkt_number;
    char *fileBuffer; // array ptr
    unsigned int fileBuffer_arraysize;
    int DestNode;

    OmnetIf_pkt();
    ~OmnetIf_pkt();
    void setFileBufferArraySize(unsigned int size);
    unsigned int getFileBufferArraySize() const;
    char getFileBuffer(unsigned int k) const;
    void setFileBuffer(unsigned int k, char fileBuffer);
};




OmnetIf_pkt::OmnetIf_pkt()
{
    pkt_number= seq_number++;
    fileBuffer_arraysize = 0;
    this->fileBuffer = NULL;
}


OmnetIf_pkt::~OmnetIf_pkt()
{
    delete [] fileBuffer;
}

void OmnetIf_pkt::setFileBufferArraySize(unsigned int size)
{
    char *fileBuffer_var2 = (size==0) ? NULL : new char[size];
    unsigned int sz = fileBuffer_arraysize < size ? fileBuffer_arraysize : size;
    for (unsigned int i=0; i<sz; i++)
        fileBuffer_var2[i] = this->fileBuffer[i];
    for (unsigned int i=sz; i<size; i++)
        fileBuffer_var2[i] = 0;
    fileBuffer_arraysize = size;
    delete [] this->fileBuffer;
    this->fileBuffer = fileBuffer_var2;
}

unsigned int OmnetIf_pkt::getFileBufferArraySize() const
{
    return fileBuffer_arraysize;
}

char OmnetIf_pkt::getFileBuffer(unsigned int k) const
{
    return fileBuffer[k];
}

void OmnetIf_pkt::setFileBuffer(unsigned int k, char fileBuffer)
{
    this->fileBuffer[k] = fileBuffer;
}


#endif // _OMNETIF_PKT_H_
