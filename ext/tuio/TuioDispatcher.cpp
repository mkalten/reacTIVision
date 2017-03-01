/*
 TUIO C++ Library
 Copyright (c) 2005-2017 Martin Kaltenbrunner <martin@tuio.org>
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3.0 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library.
*/

#include "TuioDispatcher.h"

#include <iostream>
#include <list>
#include <algorithm>
#include <cstring>

using namespace TUIO;

TuioDispatcher::TuioDispatcher() {
#ifdef WIN32
	cursorMutex = CreateMutex(NULL,FALSE,TEXT("cursorMutex"));
	objectMutex = CreateMutex(NULL,FALSE,TEXT("objectMutex"));
	blobMutex = CreateMutex(NULL,FALSE,TEXT("blobMutex"));
#else
	pthread_mutex_init(&cursorMutex,NULL);
	pthread_mutex_init(&objectMutex,NULL);
	pthread_mutex_init(&blobMutex,NULL);

#endif
}

TuioDispatcher::~TuioDispatcher() {
#ifdef WIN32
	CloseHandle(cursorMutex);
	CloseHandle(objectMutex);
	CloseHandle(blobMutex);
#else
	pthread_mutex_destroy(&cursorMutex);
	pthread_mutex_destroy(&objectMutex);
	pthread_mutex_destroy(&blobMutex);
#endif
}

void TuioDispatcher::lockObjectList() {
#ifdef WIN32
	WaitForSingleObject(objectMutex, INFINITE);
#else
	pthread_mutex_lock(&objectMutex);
#endif
}

void TuioDispatcher::unlockObjectList() {
#ifdef WIN32
	ReleaseMutex(objectMutex);
#else
	pthread_mutex_unlock(&objectMutex);
#endif
}

void TuioDispatcher::lockCursorList() {
#ifdef WIN32
	WaitForSingleObject(cursorMutex, INFINITE);
#else
	pthread_mutex_lock(&cursorMutex);
#endif
}

void TuioDispatcher::unlockCursorList() {
#ifdef WIN32
	ReleaseMutex(cursorMutex);
#else
	pthread_mutex_unlock(&cursorMutex);
#endif
}

void TuioDispatcher::lockBlobList() {
#ifdef WIN32
	WaitForSingleObject(blobMutex, INFINITE);
#else
	pthread_mutex_lock(&blobMutex);
#endif
}

void TuioDispatcher::unlockBlobList() {
#ifdef WIN32
	ReleaseMutex(blobMutex);
#else
	pthread_mutex_unlock(&blobMutex);
#endif
}

void TuioDispatcher::addTuioListener(TuioListener *listener) {
	listenerList.push_back(listener);
}

void TuioDispatcher::removeTuioListener(TuioListener *listener) {
	std::list<TuioListener*>::iterator result = find(listenerList.begin(),listenerList.end(),listener);
	if (result!=listenerList.end()) listenerList.remove(listener);
}

void TuioDispatcher::removeAllTuioListeners() {	
	listenerList.clear();
}

TuioObject* TuioDispatcher::getTuioObject(long s_id) {
	lockObjectList();
	for (std::list<TuioObject*>::iterator iter=objectList.begin(); iter != objectList.end(); iter++) {
		if((*iter)->getSessionID()==s_id) {
			unlockObjectList();
			return (*iter);
		}
	}	
	unlockObjectList();
	return NULL;
}

TuioCursor* TuioDispatcher::getTuioCursor(long s_id) {
	lockCursorList();
	for (std::list<TuioCursor*>::iterator iter=cursorList.begin(); iter != cursorList.end(); iter++) {
		if((*iter)->getSessionID()==s_id) {
			unlockCursorList();
			return (*iter);
		}
	}	
	unlockCursorList();
	return NULL;
}

TuioBlob* TuioDispatcher::getTuioBlob(long s_id) {
	lockBlobList();
	for (std::list<TuioBlob*>::iterator iter=blobList.begin(); iter != blobList.end(); iter++) {
		if((*iter)->getSessionID()==s_id) {
			unlockBlobList();
			return (*iter);
		}
	}	
	unlockBlobList();
	return NULL;
}

std::list<TuioObject*> TuioDispatcher::getTuioObjects() {
	lockObjectList();
	std::list<TuioObject*> listBuffer;
	listBuffer.insert(listBuffer.end(), objectList.begin(), objectList.end());
	//std::list<TuioObject*> listBuffer = objectList;
	unlockObjectList();
	return listBuffer;
}

std::list<TuioCursor*> TuioDispatcher::getTuioCursors() {
	lockCursorList();
	std::list<TuioCursor*> listBuffer;
	listBuffer.insert(listBuffer.end(), cursorList.begin(), cursorList.end());
	//std::list<TuioCursor*> listBuffer = cursorList;
	unlockCursorList();
	return listBuffer;
}

std::list<TuioBlob*> TuioDispatcher::getTuioBlobs() {
	lockBlobList();
	std::list<TuioBlob*> listBuffer;
	listBuffer.insert(listBuffer.end(), blobList.begin(), blobList.end());
	//std::list<TuioBlob*> listBuffer = blobList;
	unlockBlobList();
	return listBuffer;
}

std::list<TuioObject> TuioDispatcher::copyTuioObjects() {
	lockObjectList();
	std::list<TuioObject> listBuffer;
	for (std::list<TuioObject*>::iterator iter=objectList.begin(); iter != objectList.end(); iter++) {
		TuioObject *tobj = (*iter);
		listBuffer.push_back(*tobj);
	}	
	unlockObjectList();
	return listBuffer;
}

std::list<TuioCursor> TuioDispatcher::copyTuioCursors() {
	lockCursorList();
	std::list<TuioCursor> listBuffer;
	for (std::list<TuioCursor*>::iterator iter=cursorList.begin(); iter != cursorList.end(); iter++) {
		TuioCursor *tcur = (*iter);
		listBuffer.push_back(*tcur);
	}
	unlockCursorList();

	return listBuffer;
}

std::list<TuioBlob> TuioDispatcher::copyTuioBlobs() {
	lockBlobList();
	std::list<TuioBlob> listBuffer;
	for (std::list<TuioBlob*>::iterator iter=blobList.begin(); iter != blobList.end(); iter++) {
		TuioBlob *tblb = (*iter);
		listBuffer.push_back(*tblb);
	}	
	unlockBlobList();
	return listBuffer;
}



