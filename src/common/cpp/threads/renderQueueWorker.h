#ifndef RENDER_QUEUE_H
#define RENDER_QUEUE_H

#include <thread>
#include <functional>
#include <maya/MRenderView.h>
#include "queue.h"

static EventQueue::concurrent_queue<EventQueue::Event> RenderEventQueue;
EventQueue::concurrent_queue<EventQueue::Event> *theRenderEventQueue();

struct Callback{
	Callback()
	{
		millsecondInterval = 100;
		callbackId = 0;
		terminate = false;
	}
	unsigned int millsecondInterval;
	std::function<void()> functionPointer;
	unsigned int callbackId;
	bool terminate;
};

class RenderQueueWorker
{
public:
	RenderQueueWorker();
	~RenderQueueWorker();
	static void addDefaultCallbacks();
	static void removeDefaultCallbacks();
	static void reAddCallbacks();
	static void addCallbacks();
	static void removeCallbacks();
	static void startRenderQueueWorker();
	static void renderQueueWorkerTimerCallback( float time, float lastTime, void *userPtr);
	static void renderQueueWorkerNodeDirtyCallback( void *userPtr);
	static void renderQueueWorkerIdleCallback(float time, float lastTime, void *userPtr);
	static void addIdleUIComputationCreateCallback(void* data);
	static void addIdleUIComputationCallback();
	static void sceneCallback(void *);
	static void pluginUnloadCallback(void *);
	static void computationEventThread();
	static std::thread sceneThread;
	static void renderProcessThread();
	static void sendFinalizeIfQueueEmpty(void *);
	static void setStartTime();
	static void setEndTime();
	static MString getElapsedTimeString();
	static MString getCaptionString();
	static void updateRenderView(EventQueue::Event& e);
	static size_t registerCallback(std::function<void()> function, unsigned int millisecondsUpdateInterval = 100);
	static void unregisterCallback(size_t cbId);
	static void callbackWorker(size_t cbId);
private:

};

namespace EventQueue{
	struct RandomPixel{
		RV_PIXEL pixel;
		int x, y;
	};
};

std::vector<MObject> *getModifiedObjectList();

void setStartComputation();
void setEndComputation();

#endif
