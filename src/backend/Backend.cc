#include <nan.h>

#include "Backend.h"


void WaitVSync(void* arg)
{
	Backend* backend = (Backend*)arg;

	backend->waitVSync();

	// Swap buffers if it was requested
	if(!backend->listenOnPaint)
	{
		backend->swapBuffers();

		backend->listenOnPaint = true;
	}

	// Listen new `VSync` requests
	backend->waitingVSync = false;
}


Backend::Backend(string name, int width, int height)
  : name(name)
  , width(width)
  , height(height)
  , surface(NULL)
  , canvas(NULL)
	, listenOnPaint(false)
{}

Backend::~Backend()
{
  this->destroySurface();
}

void Backend::init(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  int width  = 0;
  int height = 0;
  if (info[0]->IsNumber()) width  = Nan::To<uint32_t>(info[0]).FromMaybe(0);
  if (info[1]->IsNumber()) height = Nan::To<uint32_t>(info[1]).FromMaybe(0);

  Backend *backend = construct(width, height);

  backend->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

void Backend::setCanvas(Canvas* _canvas)
{
  this->canvas = _canvas;
}


cairo_surface_t* Backend::recreateSurface()
{
  this->destroySurface();

  return this->createSurface();
}

DLL_PUBLIC cairo_surface_t* Backend::getSurface() {
  if (!surface) createSurface();
  return surface;
}

void Backend::destroySurface()
{
  if(this->surface)
  {
    cairo_surface_destroy(this->surface);
    this->surface = NULL;
  }
}


string Backend::getName()
{
  return name;
}

int Backend::getWidth()
{
  return this->width;
}
void Backend::setWidth(int width_)
{
  this->width = width_;
  this->recreateSurface();
}

int Backend::getHeight()
{
  return this->height;
}
void Backend::setHeight(int height_)
{
  this->height = height_;
  this->recreateSurface();
}

bool Backend::isSurfaceValid(){
  bool hadSurface = surface != NULL;
  bool isValid = true;

  cairo_status_t status = cairo_surface_status(getSurface());

  if (status != CAIRO_STATUS_SUCCESS) {
    error = cairo_status_to_string(status);
    isValid = false;
  }

  if (!hadSurface)
    destroySurface();

  return isValid;
}

void Backend::onPaint()
{
	if(!listenOnPaint) return;

	listenOnPaint = false;

	dispatchWaitVSync();
}

void Backend::dispatchWaitVSync()
{
	if(waitingVSync) return;

	waitingVSync = true;

	// Dispatch thread to wait for VSync
	uv_thread_create(&vSyncThread, WaitVSync, this);
}


BackendOperationNotAvailable::BackendOperationNotAvailable(Backend* backend,
  string operation_name)
  : backend(backend)
  , operation_name(operation_name)
{};

BackendOperationNotAvailable::~BackendOperationNotAvailable() throw() {};

const char* BackendOperationNotAvailable::what() const throw()
{
  std::ostringstream o;

  o << "operation " << this->operation_name;
  o << " not supported by backend " + backend->getName();

  return o.str().c_str();
};
