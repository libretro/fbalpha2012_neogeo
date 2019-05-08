#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

void init_video()
{
	Mtx view;
	Mtx model, modelview;
	Mtx44 perspective;

	f32 yscale;
	u32 xfbHeight;
	GXColor background = {0, 0, 0, 0xff};

	// Initialise the video system
	VIDEO_Init();
	
	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	
	// Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	
	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);
	
	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);

	// Flush the video register changes to the hardware
	VIDEO_Flush();

	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	// setup the fifo and then init the flipper
	void *gp_fifo = NULL;

	gp_fifo = memalign(32,GX_FIFO_MINSIZE);
	memset(gp_fifo,0,GX_FIFO_MINSIZE);
 
	GX_Init(gp_fifo,GX_FIFO_MINSIZE);

	// clears the bg to color and clears the z buffer
	GX_SetCopyClear(background, 0x00ffffff);
 
	// other gx setup
	GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
	GX_CopyDisp(xfb,GX_TRUE);

	// setup the vertex descriptor
	// tells the flipper to expect direct data
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
 	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
 
	// setup the vertex attribute table
	// describes the data
	// args: vat location 0-7, type of data, data format, size, scale
	// so for ex. in the first call we are sending position data with
	// 3 values X,Y,Z of size F32. scale sets the number of fractional
	// bits for non float data.
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGB8, 0);
 
	GX_SetNumChans(1);
	GX_SetNumTexGens(0);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

	// setup our camera at the origin
	// looking down the -z axis with y up
	guVector cam = {0.0F, 0.0F, 0.0F},
			up = {0.0F, 1.0F, 0.0F},
		  look = {0.0F, 0.0F, -1.0F};
	guLookAt(view, &cam, &up, &look);
 
	// setup our projection matrix
	// this creates a perspective matrix with a view angle of 90,
	// and aspect ratio based on the display resolution
	f32 w = rmode->viWidth;
	f32 h = rmode->viHeight;
	guPerspective(perspective, 45, (f32)w/h, 0.1F, 300.0F);
	GX_LoadProjectionMtx(perspective, GX_PERSPECTIVE);

	guMtxIdentity(model);
	guMtxTransApply(model, model, -1.5f,0.0f,-6.0f);
	guMtxConcat(view,model,modelview);
	// load the modelview matrix into matrix memory
	GX_LoadPosMtxImm(modelview, GX_PNMTX0);

	// Make the display visible
	VIDEO_SetBlack(FALSE);

	// Flush the video register changes to the hardware
	VIDEO_Flush();

	// Position cursor at row 13 and column 9
	printf("\033[13;9H%");
}

void ProgressBar(float dec, const char *msg)
{
	// Background rectangle, gradient grey
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);			// Draw A Quad
		GX_Position3f32(-1.0f, 0.1f, 0.0f);	// Top Left
		GX_Color3f32(0.1f,0.1f,0.1f);			// Set The Color To Blue
		GX_Position3f32( 4.0, 0.1f, 0.0f);		// Top Right
		GX_Color3f32(0.1f,0.1f,0.1f);			// Set The Color To Blue
		GX_Position3f32( 4.0,-0.1f, 0.0f);	// Bottom Right
		GX_Color3f32(0.0f,0.0f,0.0f);		// Set The Color To Blue
		GX_Position3f32(-1.0f,-0.1f, 0.0f);	// Bottom Left
		GX_Color3f32(0.2f,0.2f,0.2f);			// Set The Color To Blue
	GX_End();	

// Progress rectangle, light green
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);			// Draw A Quad
		GX_Position3f32(-1.0f, 0.1f, 0.0f);	// Top Left
		GX_Color3f32(0.4f,1.0f,0.4f);			// Set The Color To Blue
		GX_Position3f32( dec, 0.1f, 0.0f);		// Top Right
		GX_Color3f32(0.4f,1.0f,0.4f);			// Set The Color To Blue
		GX_Position3f32( dec,-0.1f, 0.0f);	// Bottom Right
		GX_Color3f32(0.4f,1.0f,0.4f);		// Set The Color To Blue
		GX_Position3f32(-1.0f,-0.1f, 0.0f);	// Bottom Left
		GX_Color3f32(0.4f,1.0f,0.4f);		// Set The Color To Blue
	GX_End();	

	GX_DrawDone();

  GX_SetZMode (GX_TRUE, GX_LEQUAL, GX_TRUE);
  GX_SetColorUpdate (GX_TRUE);
  GX_CopyDisp (xfb, GX_TRUE);
  GX_Flush ();

  VIDEO_SetNextFramebuffer (xfb);
  VIDEO_Flush ();
  VIDEO_WaitVSync ();

	printf("%9s %s\r",  "", msg);
}
