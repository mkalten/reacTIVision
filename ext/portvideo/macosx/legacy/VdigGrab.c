/*
 *  Created by Daniel Heckenberg.
 *  Copyright (c) 2004 Daniel Heckenberg. All rights reserved.
 *  (danielh.seeSaw<at>cse<dot>unsw<dot>edu<dot>au)
 *  
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the right to use, copy, modify, merge, publish, communicate, sublicence, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 * 
 * TO THE EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED 
 * "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT 
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#ifndef MAC_OS_X_VERSION_10_6
#include "VdigGrab.h"

#pragma mark ---- VdigGrab data ----

struct tagVdigGrab
{
	// State
	int					isPreflighted;
	int					isGrabbing;
	int					isRecording;

	// QT Components
	SeqGrabComponent	seqGrab; 
	SGChannel			sgchanVideo;
	ComponentInstance   vdCompInst;

	// Device settings
	ImageDescriptionHandle	vdImageDesc;
	Rect					vdDigitizerRect;		

	// Destination Settings
	CGrafPtr				dstPort;
	ImageSequence			dstImageSeq;

	// Compression settings
	short				cpDepth;
	CompressorComponent cpCompressor;
	CodecQ				cpSpatialQuality;
	CodecQ				cpTemporalQuality;
	long				cpKeyFrameRate;
	Fixed				cpFrameRate;
};

#pragma mark ---- Forward declarations ----

/*Boolean MySGModalFilterProc (
     DialogPtr            theDialog,
     const EventRecord    *theEvent,
     short                *itemHit,
     long                 refCon );*/

SeqGrabComponent
MakeSequenceGrabber(/*WindowRef pWindow*/);

OSErr 
MakeSequenceGrabChannel(SeqGrabComponent seqGrab, SGChannel *sgchanVideo);

OSErr
RequestSGSettings(  SeqGrabComponent		seqGrab, 
					SGChannel				sgchanVideo);

OSErr
RequestSGSettingsMinimal(  SeqGrabComponent		seqGrab, 
					SGChannel				sgchanVideo);


OSErr
RequestSGSettingsNoGUI(  SeqGrabComponent		seqGrab, 
						 SGChannel				sgchanVideo);


OSErr
RequestShowSettings(  SeqGrabComponent		seqGrab, 
					   SGChannel				sgchanVideo);

OSErr
vdgGetSettings(VdigGrab* pVdg);

#pragma mark ---- VdigGrab functions ----

VdigGrab*
vdgNew()
{
	VdigGrab* pVdg = NULL;
	pVdg = malloc(sizeof(VdigGrab));
	return pVdg;
}

OSErr
vdgInit(VdigGrab* pVdg)
{
	OSErr err;

	// zero initialize the structure
	memset(pVdg, 0, sizeof(VdigGrab));
	
	if (err = EnterMovies())
	{
		printError("EnterMovies err=%d\n", err); 
		goto endFunc;
	}

	if (!(pVdg->seqGrab = MakeSequenceGrabber(NULL)))
	{
		printError("MakeSequenceGrabber error\n"); 
		goto endFunc;
	}
	
	if (err = MakeSequenceGrabChannel(pVdg->seqGrab, &pVdg->sgchanVideo))
	{
		//printError("MakeSequenceGrabChannel err=%d\n", err); 
		goto endFunc;
	}
	
endFunc:
	return err;
}

OSErr
vdgShowSettings(VdigGrab* pVdg)
{
	OSErr err;
	
	// Use the SG Dialog to allow the user to select device and compression settings
	if (err = RequestSGSettings(  pVdg->seqGrab,
								pVdg->sgchanVideo))
	{
		//printError("RequestSGSettings err=%d\n", err); 
		goto endFunc;
	}
	
	if (err = vdgGetSettings(pVdg))
	{
		printError("vdgGetSettings err=%d\n", err); 
		goto endFunc;
	}
	
endFunc:
	return err;
}

OSErr
vdgRequestSettings(VdigGrab* pVdg)
{
	OSErr err;

	// Use the SG Dialog to allow the user to select device and compression settings
	if (err = RequestSGSettings(  pVdg->seqGrab,
								  pVdg->sgchanVideo))
	{
		//printError("RequestSGSettings err=%d\n", err); 
		goto endFunc;
	}
	
	if (err = vdgGetSettings(pVdg))
	{
		printError("vdgGetSettings err=%d\n", err); 
		goto endFunc;
	}

endFunc:
	return err;
}

OSErr
vdgRequestSettingsMinimal(VdigGrab* pVdg)
{
	OSErr err;

	// Use the SG Dialog to allow the user to select device and compression settings
	if (err = RequestSGSettingsMinimal(  pVdg->seqGrab,
								  pVdg->sgchanVideo))
	{
		//printError("RequestSGSettings err=%d\n", err); 
		goto endFunc;
	}
	
	/*if (err = vdgGetSettings(pVdg))
	{
		printError("vdgGetSettings err=%d\n", err); 
		goto endFunc;
	}*/

endFunc:
	return err;
}


OSErr
vdgRequestSettingsNoGUI(VdigGrab* pVdg)
{
	OSErr err;
		
	if (err = vdgGetSettings(pVdg))
	 {
		 //printError("vdgGetSettings err=%d\n", err); 
		 goto endFunc;
	 }
	
endFunc:
	return err;
}

OSErr
vdgGetDeviceNameAndFlags(VdigGrab* pVdg, char* szName, long* pBuffSize, UInt32* pVdFlags)
{
	OSErr err;
	Str255	vdName;
    UInt32	vdFlags;

	if (!pBuffSize)
	{
		printError("vdgGetDeviceName: NULL pointer error\n");
		err = qtParamErr; 
		goto endFunc;
	}
	
	if (err = VDGetDeviceNameAndFlags(  pVdg->vdCompInst,
										vdName,
										&vdFlags))
	{
		printError("VDGetDeviceNameAndFlags err=%d\n", err);
		*pBuffSize = 0; 
		goto endFunc;
	}
	
	if (szName)
	{
		int copyLen = min(*pBuffSize-1, vdName[0]);
			
		strncpy(szName, vdName+1, copyLen);
		szName[copyLen] = '\0';

		*pBuffSize = copyLen + 1;
	} else
	{
		*pBuffSize = vdName[0] + 1;
	} 

	if (pVdFlags)
		*pVdFlags = vdFlags;

endFunc:
	return err;
}

OSErr
vdgSetDestination(  VdigGrab* pVdg,
					CGrafPtr  dstPort )
{
	pVdg->dstPort = dstPort;
	return noErr;
}

OSErr
vdgPreflightGrabbing(VdigGrab* pVdg)
{
/* from Steve Sisak (on quicktime-api list):
A much more optimal case, if you're doing it yourself is:

  VDGetDigitizerInfo() // make sure this is a compressed source only
  VDGetCompressTypes() // tells you the supported types
  VDGetMaxSourceRect() // returns full-size rectangle (sensor size)
  VDSetDigitizerRect() // determines cropping

  VDSetCompressionOnOff(true)

    VDSetFrameRate()         // set to 0 for default
    VDSetCompression()       // compresstype=0 means default
    VDGetImageDescription()  // find out image format
    VDGetDigitizerRect()     // find out if vdig is cropping for you
    VDResetCompressSequence()

    (grab frames here)

  VDSetCompressionOnOff(false)
*/
	OSErr err;
    Rect maxRect;
	
	DigitizerInfo info;
	
	// make sure this is a compressed source only
	if (err = VDGetDigitizerInfo(pVdg->vdCompInst, &info))
	{
		if (!(info.outputCapabilityFlags & digiOutDoesCompress))
		{
			printError("VDGetDigitizerInfo: not a compressed source device.\n");
			goto endFunc;
		}
	} 
	 
//  VDGetCompressTypes() // tells you the supported types

/* 
	// Apple's SoftVDig doesn't seem to like these calls
	if (err = VDCaptureStateChanging(   pVdg->vdCompInst, 
										vdFlagCaptureLowLatency | vdFlagCaptureSetSettingsBegin))
	{
		printError("VDCaptureStateChanging err=%d\n", err);
//		goto endFunc;
	}
*/

	if (err = VDGetMaxSrcRect(  pVdg->vdCompInst, currentIn, &maxRect))
	{
		printError("VDGetMaxSrcRect err=%d\n", err);
//		goto endFunc;
	}
	
	// Try to set maximum capture size ... is this necessary as we're setting the 
	// rectangle in the VDSetCompression call later?  I suppose that it is, as
	// we're setting digitization size rather than compression size here...
	// Apple vdigs don't like this call	
	if (err = VDSetDigitizerRect( pVdg->vdCompInst, &maxRect))
	{
		printError("VDSetDigitizerRect err=%d\n", err);
//		goto endFunc;		
	}

	if (err = VDSetCompressionOnOff( pVdg->vdCompInst, 1))
	{
		printError("VDSetCompressionOnOff err=%d\n", err);
//		goto endFunc;		
	}

	// We could try to force the frame rate here... necessary for ASC softvdig
	if (err = VDSetFrameRate( pVdg->vdCompInst, 0))
	{
		printError("VDSetFrameRate err=%d\n", err);
//		goto endFunc;		
	}

	// try to set a format that matches our target
	// necessary for ASC softvdig (even if it doesn't support
	// the requested codec)
	// note that for the Apple IIDC vdig in 10.3 if we request yuv2 explicitly
	// we'll get 320x240 frames returned but if we leave codecType as 0
	// we'll get 640x480 frames returned instead (which use 4:1:1 encoding on
	// the wire rather than 4:2:2)
    if (err = VDSetCompression(	pVdg->vdCompInst,
                                0, //'yuv2'
                                0,	
                                &maxRect, 
                                0, //codecNormalQuality,
                                0, //codecNormalQuality,
                                0))
	{
		printError("VDSetCompression err=%d\n", err);
//		goto endFunc;			
	}

/*
	if (err = VDCaptureStateChanging(pVdg->vdCompInst,
   									vdFlagCaptureLowLatency | vdFlagCaptureSetSettingsEnd))
	{
		printError("VDCaptureStateChanging err=%d\n", err);
//		goto endFunc;	   
	}
 */     
	if (err =  VDResetCompressSequence( pVdg->vdCompInst ))
	{
//		printError("VDResetCompressSequence err=%d\n", err);
//		goto endFunc;	   
   }
 
   pVdg->vdImageDesc = (ImageDescriptionHandle)NewHandle(0);
   if (err = VDGetImageDescription( pVdg->vdCompInst, pVdg->vdImageDesc))
   {
		printError("VDGetImageDescription err=%d\n", err);
//		goto endFunc;	   
   }

	// From Steve Sisak: find out if Digitizer is cropping for you.
	if (err = VDGetDigitizerRect( pVdg->vdCompInst, &pVdg->vdDigitizerRect))
	{
		printError("VDGetDigitizerRect err=%d\n", err);
//		goto endFunc;
	}

	pVdg->isPreflighted = 1;
	
endFunc:
	return err;
}

OSErr
vdgStartGrabbing(VdigGrab*   pVdg)
{
	OSErr err;

	if (!pVdg->isPreflighted)
	{
		printError("vdgStartGrabbing called without previous successful vdgPreflightGrabbing()\n");
		err = badCallOrderErr; 
		goto endFunc;	
	}

    if (err = VDCompressOneFrameAsync( pVdg->vdCompInst ))
	{
		printError("VDCompressOneFrameAsync err=%d\n", err);
		goto endFunc;	
	}
	
	if (err = vdgDecompressionSequenceBegin( pVdg, pVdg->dstPort, NULL, NULL ))
	{
		printError("vdgDecompressionSequenceBegin err=%d\n", err);
		goto endFunc;	
	}
	
	pVdg->isGrabbing = 1;

endFunc:
	return err;
}

OSErr
vdgGetDataRate( VdigGrab*   pVdg, 
				long*		pMilliSecPerFrame,
				Fixed*      pFramesPerSecond,
				long*       pBytesPerSecond)
{
	OSErr err;

	if (err = VDGetDataRate( pVdg->vdCompInst, 
							  pMilliSecPerFrame,
							  pFramesPerSecond,
							  pBytesPerSecond))
	{
		printError("VDGetDataRate err=%d\n", err);
		goto endFunc;		
	}
	
endFunc:	
	return err;
}

OSErr
vdgGetImageDescription( VdigGrab* pVdg,
						ImageDescriptionHandle vdImageDesc )
{
	OSErr err;
	
	if (err = VDGetImageDescription( pVdg->vdCompInst, vdImageDesc))
	{
		printError("VDGetImageDescription err=%d\n", err);
		goto endFunc;		
	}
	
endFunc:	
	return err;
}

OSErr
vdgDecompressionSequenceBegin(  VdigGrab* pVdg,
								CGrafPtr dstPort, 
								Rect* pDstRect,
								MatrixRecord* pDstScaleMatrix )
{
	OSErr err;
	
// 	Rect				   sourceRect = pMungData->bounds;
//	MatrixRecord		   scaleMatrix;	

  	// !HACK! Different conversions are used for these two equivalent types
	// so we force the cType so that the more efficient path is used
	if ((*pVdg->vdImageDesc)->cType == FOUR_CHAR_CODE('yuv2'))
		(*pVdg->vdImageDesc)->cType = FOUR_CHAR_CODE('yuvu'); // kYUVUPixelFormat

	// make a scaling matrix for the sequence
//	sourceRect.right = (*pVdg->vdImageDesc)->width;
//	sourceRect.bottom = (*pVdg->vdImageDesc)->height;
//	RectMatrix(&scaleMatrix, &sourceRect, &pMungData->bounds);
			
    // begin the process of decompressing a sequence of frames
    // this is a set-up call and is only called once for the sequence - the ICM will interrogate different codecs
    // and construct a suitable decompression chain, as this is a time consuming process we don't want to do this
    // once per frame (eg. by using DecompressImage)
    // for more information see Ice Floe #8 http://developer.apple.com/quicktime/icefloe/dispatch008.html
    // the destination is specified as the GWorld
	if (err = DecompressSequenceBeginS(    &pVdg->dstImageSeq,	// pointer to field to receive unique ID for sequence
										  pVdg->vdImageDesc,	// handle to image description structure
										  0,
										  0,
										  dstPort,				// port for the DESTINATION image
										  NULL,					// graphics device handle, if port is set, set to NULL
										  NULL,					// source rectangle defining the portion of the image to decompress 
                                          NULL,					// transformation matrix
                                          srcCopy,				// transfer mode specifier
                                          (RgnHandle)NULL,		// clipping region in dest. coordinate system to use as a mask
                                          NULL,					// flags
                                          codecNormalQuality,	//codecHighQuality,					// accuracy in decompression
                                          bestSpeedCodec))		//anyCodec); //bestSpeedCodec);		// compressor identifier or special identifiers ie. bestSpeedCodec
	{
		printError("DecompressSequenceBeginS err=%d\n", err);
		goto endFunc;
	}
		  
endFunc:	
	return err;
}

OSErr
vdgDecompressionSequenceWhen(   VdigGrab* pVdg,
								Ptr theData,
								long dataSize)
{
	OSErr err;
	CodecFlags	ignore = 0;
  
	if(err = DecompressSequenceFrameWhen(   pVdg->dstImageSeq,	// sequence ID returned by DecompressSequenceBegin
											theData,			// pointer to compressed image data
											dataSize,			// size of the buffer
											0,					// in flags
											&ignore,			// out flags
											NULL,				// async completion proc
											NULL ))
	{
		printError("DecompressSequenceFrameWhen err=%d\n", err);
		goto endFunc;
	}
	
endFunc:
	return err;
}

OSErr
vdgDecompressionSequenceEnd( VdigGrab* pVdg )
{
	OSErr err;

	if (!pVdg->dstImageSeq)
	{
		printError("vdgDestroyDecompressionSequence NULL sequence\n");
		err = qtParamErr; 
		goto endFunc;
	}

	if (err = CDSequenceEnd(pVdg->dstImageSeq))
	{
		printError("CDSequenceEnd err=%d\n", err);
		goto endFunc;
	}
	
	pVdg->dstImageSeq = 0;

endFunc:
	return err;
}

OSErr
vdgStopGrabbing(VdigGrab* pVdg)
{
	OSErr err;

	if (err = VDSetCompressionOnOff( pVdg->vdCompInst, 0))
	{
		printError("VDSetCompressionOnOff err=%d\n", err);
//		goto endFunc;		
	}

	if (err = vdgDecompressionSequenceEnd(pVdg))
	{
		printError("vdgDecompressionSequenceEnd err=%d\n", err);
//		goto endFunc;
	}

	pVdg->isGrabbing = 0;

//endFunc:
	return err;
}

bool
vdgIsGrabbing(VdigGrab* pVdg)
{
	return pVdg->isGrabbing;
}

OSErr
vdgIdle(VdigGrab* pVdg, int*  pIsUpdated)
{
	OSErr err;

    UInt8 		queuedFrameCount;
    Ptr			theData;
    long		dataSize;
    UInt8		similarity;
    TimeRecord	time;

	*pIsUpdated = 0;

	// should be while?
	if ( !(err = vdgPoll( pVdg,
						&queuedFrameCount,
						&theData,
						&dataSize,
						&similarity,
						&time))
			&& queuedFrameCount)
	{
		*pIsUpdated = 1;
		
		// Decompress the sequence
		if (err = vdgDecompressionSequenceWhen( pVdg,
												theData,
												dataSize))
		{
			printError("vdgDecompressionSequenceWhen err=%d\n", err);
			//return err;
		}
		
		// return the buffer
		if(err = vdgReleaseBuffer(pVdg, theData))
		{
			printError("vdgReleaseBuffer err=%d\n", err);
			return err;
		}
	} 
	
	if (err) printError("vdgPoll err=%d\n", err);
	return err;
}
	

OSErr
vdgPoll(	VdigGrab*   pVdg,
			UInt8*		pQueuedFrameCount,
			Ptr*		pTheData,
			long*		pDataSize,
			UInt8*		pSimilarity,
			TimeRecord*	pTime )
{
	OSErr err;

	if (!pVdg->isGrabbing)
	{ 
		printError("vdgGetFrame error: not grabbing\n");
		err = qtParamErr; 
		goto endFunc;
	}
	
    if (err = VDCompressDone(	pVdg->vdCompInst,
								pQueuedFrameCount,
								pTheData,
								pDataSize,
								pSimilarity,
								pTime ))
	{
		printError("vdgGetFrame error: not grabbing\n");
		goto endFunc;
	}
	
	// Overlapped grabbing
    if (*pQueuedFrameCount)
    {
		if (err = VDCompressOneFrameAsync(pVdg->vdCompInst))
		{
			printError("VDCompressOneFrameAsync err=%d\n", err);
			goto endFunc;		
		}
	}

endFunc:
	return err;
}

OSErr
vdgReleaseBuffer(VdigGrab*   pVdg, Ptr theData)
{
	OSErr err;

	if (err = VDReleaseCompressBuffer(pVdg->vdCompInst, theData))
	{
		printError("VDReleaseCompressBuffer err=%d\n", err);
		goto endFunc;		
	}

endFunc:
	return err;
}

OSErr
vdgUninit(VdigGrab* pVdg)
{
	OSErr err = noErr;		

	if (pVdg->vdImageDesc)
	{
		DisposeHandle((Handle)pVdg->vdImageDesc);
		pVdg->vdImageDesc = nil;
	}

	if (pVdg->vdCompInst)
	{
		if (err = CloseComponent(pVdg->vdCompInst))
			//printError("CloseComponent err=%d\n", err);		
		pVdg->vdCompInst = nil;
	}

	if (pVdg->sgchanVideo)
	{
		if (err = SGDisposeChannel(pVdg->seqGrab, pVdg->sgchanVideo))
			printError("SGDisposeChannel err=%d\n", err);	
		pVdg->sgchanVideo = nil;
	}
	
	if (pVdg->seqGrab)
	{
		if (err = CloseComponent(pVdg->seqGrab))
			printError("CloseComponent err=%d\n", err);		
		pVdg->seqGrab = nil;
	}
	
	ExitMovies();
	return err;
}

void
vdgDelete(VdigGrab* pVdg)
{
	if (!pVdg)
	{
		printError("vdgDelete NULL pointer\n");
		return;
	}
	
	free(pVdg); 
}

#pragma mark ---- Vdig utility functions ----

OSErr
vdgGetSettings(VdigGrab* pVdg)
{	
	OSErr err;

	// Extract information from the SG
    if (err = SGGetVideoCompressor (	pVdg->sgchanVideo, 
										&pVdg->cpDepth,
										&pVdg->cpCompressor,
										&pVdg->cpSpatialQuality, 
										&pVdg->cpTemporalQuality, 
										&pVdg->cpKeyFrameRate ))
	{
		printError("SGGetVideoCompressor err=%d\n", err);
		goto endFunc;
	}
    
	if (err = SGGetFrameRate(pVdg->sgchanVideo, &pVdg->cpFrameRate))                                                                 
	{
		printError("SGGetFrameRate err=%d\n", err);
		goto endFunc;
	}
	
	// Get the selected vdig from the SG
    if (!(pVdg->vdCompInst = SGGetVideoDigitizerComponent(pVdg->sgchanVideo)))
    {
		printError("SGGetVideoDigitizerComponent error\n");
		goto endFunc;
	}
	
endFunc:
	return err;
}

int vdgGetFrameRate (VdigGrab* pVdg) {
	return (int)(pVdg->cpFrameRate/65536);
}

// --------------------
// MakeSequenceGrabber  (adapted from Apple mung sample)
//
SeqGrabComponent
MakeSequenceGrabber(/*WindowRef pWindow*/)
{
	SeqGrabComponent seqGrab = NULL;
	OSErr			 err = noErr;

	// open the default sequence grabber
    if(!(seqGrab = OpenDefaultComponent(SeqGrabComponentType, 0)))
	{
		printError("OpenDefaultComponent failed to open the default sequence grabber.\n");
		goto endFunc;
	}
	
   	// initialize the default sequence grabber component
   	if(err = SGInitialize(seqGrab))
	{
		printError("SGInitialize err=%d\n", err);
		goto endFunc;
	}
	
	// This should be defaulted to the current port according to QT doco
	/*if (err = SGSetGWorld(seqGrab, GetWindowPort(pWindow), NULL))
	{
		printError("SGSetGWorld err=%d\n", err);
		goto endFunc;
	}*/
	
	// specify the destination data reference for a record operation
	// tell it we're not making a movie
	// if the flag seqGrabDontMakeMovie is used, the sequence grabber still calls
	// your data function, but does not write any data to the movie file
	// writeType will always be set to seqGrabWriteAppend
  	if (err = SGSetDataRef(seqGrab,
    						   0,
    						   0,
    						   seqGrabDontMakeMovie))
    {
		printError("SGSetGWorld err=%d\n", err);
		goto endFunc;
	}

endFunc:	
    if (err && (seqGrab != NULL)) 
	{ // clean up on failure
    	CloseComponent(seqGrab);
        seqGrab = NULL;
    }
    
	return seqGrab;
}


// --------------------
// MakeSequenceGrabChannel (adapted from Apple mung sample)
//
OSErr 
MakeSequenceGrabChannel(SeqGrabComponent seqGrab, SGChannel* psgchanVideo)
{
    long  flags = 0;
    OSErr err = noErr;
    
    if (err = SGNewChannel(seqGrab, VideoMediaType, psgchanVideo))
	{
		//printError("SGNewChannel err=%d\n", err);
		goto endFunc;
	}
	
//	err = SGSetChannelBounds(*sgchanVideo, rect);
   	// set usage for new video channel to avoid playthrough
	// note we don't set seqGrabPlayDuringRecord
	if (err = SGSetChannelUsage(*psgchanVideo, flags | seqGrabRecord))
	{
		printError("SGSetChannelUsage err=%d\n", err);
		goto endFunc;
	}

endFunc:
    if ((err != noErr) && psgchanVideo) {
        // clean up on failure
        SGDisposeChannel(seqGrab, *psgchanVideo);
        *psgchanVideo = NULL;
    }
	
	return err;
}

OSErr
RequestSGSettingsMinimal(  SeqGrabComponent		seqGrab, 
					SGChannel				sgchanVideo )
{
	OSErr err;
    	            
    /*SGModalFilterUPP MySGModalFilterUPP = NULL;
	if (!(MySGModalFilterUPP = NewSGModalFilterUPP (MySGModalFilterProc)))
	{
		printError("NewSGModalFilterUPP error\n");
		err = -1; // TODO appropriate error code
		goto endFunc;
	}*/
 
	
Component *PanelListPtr = NULL;
UInt8 NumberOfPanels = 0;

ComponentDescription cd = { SeqGrabPanelType, VideoMediaType, 0, 0, 0 };
Component c = 0;
Component *cPtr = NULL;

// set up the settings panel list removing the "Compression" panel
NumberOfPanels = CountComponents(&cd);
PanelListPtr = (Component *)NewPtr(sizeof(Component) * (NumberOfPanels + 1));

cPtr = PanelListPtr;
NumberOfPanels = 0;
do {
	ComponentDescription compInfo;
	c = FindNextComponent(c, &cd);
	if (c) {
		Handle hName = NewHandle(0);
		GetComponentInfo(c, &compInfo, hName, NULL, NULL);
		
			char *cName = *hName;
			int len = *cName++;
			cName[len] = 0;

		if ((strcmp( cName,"Compression") != 0) && (strcmp( cName,"Source") != 0)) {
			*cPtr++ = c;
			NumberOfPanels++;		
		} 
		DisposeHandle(hName);
	}
} while (c);
			    
	// let the user configure and choose the device and settings
	// "Due to a bug in all versions QuickTime 6.x for the function call "SGSettingsDialog()" 
	// when used with the "seqGrabSettingsPreviewOnly" parameter, all third party panels will 
	// be excluded."  	from http://www.outcastsoft.com/ASCDFG1394.html  15/03/04
	//if (err = SGSettingsDialog(seqGrab, sgchanVideo, 0, NULL, seqGrabSettingsPreviewOnly, MySGModalFilterUPP, 0))
	//if (err = SGSettingsDialog(seqGrab, sgchanVideo, 0, NULL, 0, MySGModalFilterUPP, 0))
	if (err = SGSettingsDialog(seqGrab, sgchanVideo, NumberOfPanels, PanelListPtr, seqGrabSettingsPreviewOnly, NULL, 0))
	{
		//printError("SGSettingsDialog err=%d\n", err);
		goto endFunc;
	}
    
	// Dispose of the UPP
	/* if (MySGModalFilterUPP)
		DisposeSGModalFilterUPP(MySGModalFilterUPP);*/
    
endFunc:
	return err;
}

OSErr
RequestSGSettings(  SeqGrabComponent		seqGrab, 
					SGChannel				sgchanVideo )
{
	Component *PanelListPtr = NULL;
	UInt8 NumberOfPanels = 0;

	ComponentDescription cd = { SeqGrabPanelType, VideoMediaType, 0, 0, 0 };
	Component c = 0;
	Component *cPtr = NULL;
	Component source = 0;

	// set up the settings panel list removing the "Compression" panel
	NumberOfPanels = CountComponents(&cd);
	PanelListPtr = (Component *)NewPtr(sizeof(Component) * (NumberOfPanels + 1));

	cPtr = PanelListPtr;
	NumberOfPanels = 0;
	do {
		ComponentDescription compInfo;
		c = FindNextComponent(c, &cd);
		if (c) {
			Handle hName = NewHandle(0);
			GetComponentInfo(c, &compInfo, hName, NULL, NULL);
			char *cName = *hName;
			int len = *cName++;
			cName[len] = 0;

			if ((strcmp(cName,"Compression") != 0) && (strcmp(cName,"Source") != 0)) {
				*cPtr++ = c;
				NumberOfPanels++;
			} else if (strcmp(cName, "Source") == 0) {
				source = c;
			}
			DisposeHandle(hName);
		}

	} while (c);

	*cPtr++ = PanelListPtr[0];
	PanelListPtr[0] = source;
	NumberOfPanels++;
	
	 		    
	// let the user configure and choose the device and settings
	// "Due to a bug in all versions QuickTime 6.x for the function call "SGSettingsDialog()" 
	// when used with the "seqGrabSettingsPreviewOnly" parameter, all third party panels will 
	// be excluded."  	from http://www.outcastsoft.com/ASCDFG1394.html  15/03/04
	//if (err = SGSettingsDialog(seqGrab, sgchanVideo, 0, NULL, seqGrabSettingsPreviewOnly, MySGModalFilterUPP, 0))
	//if (err = SGSettingsDialog(seqGrab, sgchanVideo, 0, NULL, 0, MySGModalFilterUPP, 0))
	OSErr err = SGSettingsDialog(seqGrab, sgchanVideo, NumberOfPanels, PanelListPtr, 0, NULL, 0);
	return err;
}

/*
// From QT sample code
// Declaration of a typical application-defined function
Boolean MySGModalFilterProc (
     DialogPtr            theDialog,
     const EventRecord    *theEvent,
     short                *itemHit,
     long                 refCon )
{
	// Ordinarily, if we had multiple windows we cared about, we'd handle
	// updating them in here, but since we don't, we'll just clear out
	// any update events meant for us
	Boolean	handled = false;
	
	if ((theEvent->what == updateEvt) && 
		((WindowPtr) theEvent->message == (WindowPtr) refCon))
	{
		BeginUpdate ((WindowPtr) refCon);
		EndUpdate ((WindowPtr) refCon);
		handled = true;
	}
	return (handled);
}
*/

#pragma mark ---- Public utility functions ----

OSErr
createOffscreenGWorld(  GWorldPtr* pGWorldPtr,
						OSType		pixelFormat,
						Rect*		pBounds)
{   
	OSErr err;
	CGrafPtr theOldPort;
    GDHandle theOldDevice;
	
    // create an offscreen GWorld 
    if (err = QTNewGWorld(  pGWorldPtr,				// returned GWorld
    					pixelFormat,			// pixel format
    					pBounds,				// bounds
    					0,						// color table
    					NULL,					// GDHandle
    					0))						// flags
	{
		printError("QTNewGWorld: err=%d\n", err);
		goto endFunc;
	}
   
    // lock the pixmap and make sure it's locked because
    // we can't decompress into an unlocked PixMap
    if(!LockPixels(GetGWorldPixMap(*pGWorldPtr)))
		printError("createOffscreenGWorld: Can't lock pixels!\n");
    
    GetGWorld(&theOldPort, &theOldDevice);    
    SetGWorld(*pGWorldPtr, NULL);
    BackColor(blackColor);
    ForeColor(whiteColor);
    EraseRect(pBounds);    
    SetGWorld(theOldPort, theOldDevice);

endFunc:
	return err;
}

void
disposeOffscreenGWorld(GWorldPtr gworld)
{

	UnlockPixels(GetGWorldPixMap(gworld));
    DisposeGWorld(gworld);
}

#pragma mark ---- utility functions ----

void
printError(const char* format, ...)
{
    va_list arglist;
	fprintf(stderr, "vdigGrab: ");
    va_start( arglist, format );
    vfprintf( stderr, format, arglist );
    va_end( arglist );
}

long inline
min (long l1, long l2)
{
	return (l1 < l2) ? l1 : l2;
}
#endif

