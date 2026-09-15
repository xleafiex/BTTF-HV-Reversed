#include "common.h"

#include "NodeName.h"

static int32 gPluginOffset;
static const int32 NodeNameCapacity = 128;

enum
{
	ID_NODENAME = MAKECHUNKID(rwVENDORID_ROCKSTAR, 0xFE),
};

#define NODENAMEEXT(o) (RWPLUGINOFFSET(char, o, gPluginOffset))

void*
NodeNameConstructor(void *object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
	if(gPluginOffset > 0)
		NODENAMEEXT(object)[0] = '\0';
	return object;
}

void*
NodeNameDestructor(void *object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
	return object;
}

void*
NodeNameCopy(void *dstObject, const void *srcObject, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
	strncpy(NODENAMEEXT(dstObject), NODENAMEEXT(srcObject), NodeNameCapacity - 1);
	NODENAMEEXT(dstObject)[NodeNameCapacity - 1] = '\0';
	return nil;
}

RwStream*
NodeNameStreamRead(RwStream *stream, RwInt32 binaryLength, void *object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
	if (binaryLength < 0) return nil;
	int32 count = binaryLength < NodeNameCapacity ? binaryLength : NodeNameCapacity - 1;
	RwStreamRead(stream, NODENAMEEXT(object), count);
	NODENAMEEXT(object)[count] = '\0';
	if (binaryLength > count) RwStreamSkip(stream, binaryLength - count);
	return stream;
}

RwStream*
NodeNameStreamWrite(RwStream *stream, RwInt32 binaryLength, const void *object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
	RwStreamWrite(stream, NODENAMEEXT(object), binaryLength);
	return stream;
}

RwInt32
NodeNameStreamGetSize(const void *object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
	char *name = NODENAMEEXT(object);	// can't be nil
	return name ? (RwInt32)rwstrlen(name) : 0;
}

bool
NodeNamePluginAttach(void)
{
	gPluginOffset = RwFrameRegisterPlugin(NodeNameCapacity, ID_NODENAME,
                                NodeNameConstructor,
                                NodeNameDestructor,
                                NodeNameCopy);
	RwFrameRegisterPluginStream(ID_NODENAME,
		NodeNameStreamRead,
		NodeNameStreamWrite,
		NodeNameStreamGetSize);
	return gPluginOffset != -1;
}

char*
GetFrameNodeName(RwFrame *frame)
{
	if(gPluginOffset < 0)
		return nil;
	return NODENAMEEXT(frame);
}
