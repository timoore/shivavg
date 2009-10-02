/*
 * Copyright (c) 2007 Ivan Leben
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library in the file COPYING;
 * if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#define VG_API_EXPORT
#include "openvg.h"
#include "shContext.h"
#include "shParams.h"
#include <string.h>
#include <stdio.h>

/*-----------------------------------------------------
 * Simple functions to create a VG context instance
 * on top of an existing OpenGL context.
 * TODO: There is no mechanics yet to asure the OpenGL
 * context exists and to choose which context / window
 * to bind to. 
 *-----------------------------------------------------*/

static VGContext *g_context = NULL;

VG_API_CALL VGboolean vgCreateContextSH(VGint width, VGint height)
{
  /* return if already created */
  if (g_context) return VG_TRUE;
  
  /* create new context */
  SH_NEWOBJ(VGContext, g_context);
  if (!g_context) return VG_FALSE;
  
  /* init surface info */
  g_context->surfaceWidth = width;
  g_context->surfaceHeight = height;
  
  /* setup GL projection */
  glViewport(0,0,width,height);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0,width,0,height);
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  /* depth buffer clear value for scissor test */
  glClearDepth(0.0);
  glDepthMask(GL_FALSE);
  glDisable(GL_DEPTH_TEST);
  
  return VG_TRUE;
}

VG_API_CALL void vgResizeSurfaceSH(VGint width, VGint height)
{
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  /* update surface info */
  context->surfaceWidth = width;
  context->surfaceHeight = height;
  
  /* setup GL projection */
  glViewport(0,0,width,height);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0,width,0,height);
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  
  VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void vgDestroyContextSH()
{
  /* return if already released */
  if (!g_context) return;
  
  /* delete context object */
  SH_DELETEOBJ(VGContext, g_context);
  g_context = NULL;
}

VGContext* shGetContext()
{
  SH_ASSERT(g_context);
  return g_context;
}

/*-----------------------------------------------------
 * VGContext constructor
 *-----------------------------------------------------*/

void shLoadExtensions(VGContext *c);

void VGContext_ctor(VGContext *c)
{
  /* Surface info */
  c->surfaceWidth = 0;
  c->surfaceHeight = 0;
  
  /* GetString info */
  strncpy(c->vendor, "Ivan Leben", sizeof(c->vendor));
  strncpy(c->renderer, "ShivaVG 0.1.0", sizeof(c->renderer));
  strncpy(c->version, "1.0", sizeof(c->version));
  strncpy(c->extensions, "", sizeof(c->extensions));
  
  /* Mode settings */
  c->matrixMode = VG_MATRIX_PATH_USER_TO_SURFACE;
  c->fillRule = VG_EVEN_ODD;
  c->imageQuality = VG_IMAGE_QUALITY_FASTER;
  c->renderingQuality = VG_RENDERING_QUALITY_BETTER;
  c->blendMode = VG_BLEND_SRC_OVER;
  c->imageMode = VG_DRAW_IMAGE_NORMAL;
  
  /* Scissor rectangles */
  SH_INITOBJ(SHVector3Array, c->scissor);
  SH_INITOBJ(SHUint16Array, c->scissorIndices);
  c->scissoring = VG_FALSE;
  c->masking = VG_FALSE;
  
  /* Stroke parameters */
  c->strokeLineWidth = 1.0f;
  c->strokeCapStyle = VG_CAP_BUTT;
  c->strokeJoinStyle = VG_JOIN_MITER;
  c->strokeMiterLimit = 4.0f;
  c->strokeDashPhase = 0.0f;
  c->strokeDashPhaseReset = VG_FALSE;
  SH_INITOBJ(SHFloatArray, c->strokeDashPattern);
  
  /* Edge fill color for vgConvolve and pattern paint */
  CSET(c->tileFillColor, 0,0,0,0);
  
  /* Color for vgClear */
  CSET(c->clearColor, 0,0,0,0);
  
  /* Color components layout inside pixel */
  c->pixelLayout = VG_PIXEL_LAYOUT_UNKNOWN;
  
  /* Source format for image filters */
  c->filterFormatLinear = VG_FALSE;
  c->filterFormatPremultiplied = VG_FALSE;
  c->filterChannelMask = VG_RED|VG_GREEN|VG_BLUE|VG_ALPHA;
  
  /* Matrices */
  SH_INITOBJ(SHMatrix3x3, c->pathTransform);
  SH_INITOBJ(SHMatrix3x3, c->imageTransform);
  SH_INITOBJ(SHMatrix3x3, c->fillTransform);
  SH_INITOBJ(SHMatrix3x3, c->strokeTransform);
  
  /* Paints */
  c->fillPaint = NULL;
  c->strokePaint = NULL;
  SH_INITOBJ(SHPaint, c->defaultPaint);
  
  /* Error */
  c->error = VG_NO_ERROR;
  
  /* Resources */
  SH_INITOBJ(SHPathArray, c->paths);
  SH_INITOBJ(SHPaintArray, c->paints);
  SH_INITOBJ(SHImageArray, c->images);

  shLoadExtensions(c);
}

/*-----------------------------------------------------
 * VGContext constructor
 *-----------------------------------------------------*/

void VGContext_dtor(VGContext *c)
{
  int i;
  
  SH_DEINITOBJ(SHVector3Array, c->scissor);
  SH_DEINITOBJ(SHUint16Array, c->scissorIndices);
  SH_DEINITOBJ(SHFloatArray, c->strokeDashPattern);
  
  /* Destroy resources */
  for (i=0; i<c->paths.size; ++i)
    SH_DELETEOBJ(SHPath, c->paths.items[i]);
  
  for (i=0; i<c->paints.size; ++i)
    SH_DELETEOBJ(SHPaint, c->paints.items[i]);
  
  for (i=0; i<c->images.size; ++i)
    SH_DELETEOBJ(SHImage, c->images.items[i]);
}

/*--------------------------------------------------
 * Tries to find resources in this context
 *--------------------------------------------------*/

SHint shIsValidPath(VGContext *c, VGHandle h)
{
  int index = shPathArrayFind(&c->paths, (SHPath*)h);
  return (index == -1) ? 0 : 1;
}

SHint shIsValidPaint(VGContext *c, VGHandle h)
{
  int index = shPaintArrayFind(&c->paints, (SHPaint*)h);
  return (index == -1) ? 0 : 1;
}

SHint shIsValidImage(VGContext *c, VGHandle h)
{
  int index = shImageArrayFind(&c->images, (SHImage*)h);
  return (index == -1) ? 0 : 1;
}

/*--------------------------------------------------
 * Tries to find a resources in this context and
 * return its type or invalid flag.
 *--------------------------------------------------*/

SHResourceType shGetResourceType(VGContext *c, VGHandle h)
{
  if (shIsValidPath(c, h))
    return SH_RESOURCE_PATH;
  
  else if (shIsValidPaint(c, h))
    return SH_RESOURCE_PAINT;
  
  else if (shIsValidImage(c, h))
    return SH_RESOURCE_IMAGE;
  
  else
    return SH_RESOURCE_INVALID;
}

/*-----------------------------------------------------
 * Sets the specified error on the given context if
 * there is no pending error yet
 *-----------------------------------------------------*/

void shSetError(VGContext *c, VGErrorCode e)
{
  if (c->error == VG_NO_ERROR)
    c->error = e;
}

/*--------------------------------------------------
 * Returns the oldest error pending on the current
 * context and clears its error code
 *--------------------------------------------------*/

VG_API_CALL VGErrorCode vgGetError(void)
{
  VGErrorCode error;
  VG_GETCONTEXT(VG_NO_CONTEXT_ERROR);
  error = context->error;
  context->error = VG_NO_ERROR;
  VG_RETURN(error);
}

VG_API_CALL void vgFlush(void)
{
  VG_GETCONTEXT(VG_NO_RETVAL);
  glFlush();
  VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void vgFinish(void)
{
  VG_GETCONTEXT(VG_NO_RETVAL);
  glFinish();
  VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void vgMask(VGImage mask, VGMaskOperation operation,
                        VGint x, VGint y, VGint width, VGint height)
{
}

static void makeRectangle(VGfloat x, VGfloat y, VGfloat w, VGfloat h, VGfloat z,
                          SHVector3* coords, SHuint16* indices,
                          SHuint16 indexBase)
{
  if (coords) {
    coords->x = x;  coords->y = y;  coords->z = z;
    coords++;
    coords->x = x + w;  coords->y = y;  coords->z = z;
    coords++;
    coords->x = x + w;  coords->y = y + h;  coords->z = z;
    coords++;
    coords->x = x;  coords->y = y + h;  coords->z = z;
  }
  if (indices) {
    indices[0] = indexBase;
    indices[1] = indexBase + 1;
    indices[2] = indexBase + 2;
    indices[3] = indexBase;
    indices[4] = indexBase + 2;
    indices[5] = indexBase + 3;
  }
}

VG_API_CALL void vgClear(VGint x, VGint y, VGint width, VGint height)
{
  SHVector3 clearVerts[4];
  static SHuint16 clearIdx[6] = {0, 1, 2, 0, 2, 3};
  
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  /* Clip to window */
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (width > context->surfaceWidth) width = context->surfaceWidth;
  if (height > context->surfaceHeight) height = context->surfaceHeight;
  /* Check if scissoring needed */
  if (context->scissoring == VG_FALSE) {
    if (x > 0 || y > 0 ||
        width < context->surfaceWidth ||
        height < context->surfaceHeight) {
    
      glScissor(x, y, width, height);
      glEnable(GL_SCISSOR_TEST);
    }
    glClearColor(context->clearColor.r,
                 context->clearColor.g,
                 context->clearColor.b,
                 context->clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
    VG_RETURN(VG_NO_RETVAL);
  }
  
  makeRectangle((VGfloat)x, (VGfloat)y, (VGfloat)width, (VGfloat)height,
                0.0f, clearVerts, 0, 0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  /* Clear GL color buffer */
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, clearVerts);
  glColor4fv(&context->clearColor.r);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, clearIdx);
  glDisableClientState(GL_VERTEX_ARRAY);

  /* TODO: what about stencil and depth? when do we clear that?
     we would need some kind of special "begin" function at
     beginning of each drawing or clear the planes prior to each
     drawing where it takes places */
  
  VG_RETURN(VG_NO_RETVAL);
}


/*-----------------------------------------------------------
 * Returns the matrix currently selected via VG_MATRIX_MODE
 *-----------------------------------------------------------*/

SHMatrix3x3* shCurrentMatrix(VGContext *c)
{
  switch(c->matrixMode) {
  case VG_MATRIX_PATH_USER_TO_SURFACE:
    return &c->pathTransform;
  case VG_MATRIX_IMAGE_USER_TO_SURFACE:
    return &c->imageTransform;
  case VG_MATRIX_FILL_PAINT_TO_USER:
    return &c->fillTransform;
  default:
    return &c->strokeTransform;
  }
}

/*--------------------------------------
 * Sets the current matrix to identity
 *--------------------------------------*/

VG_API_CALL void vgLoadIdentity(void)
{
  SHMatrix3x3 *m;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  m = shCurrentMatrix(context);
  IDMAT((*m));
  
  VG_RETURN(VG_NO_RETVAL);
}

/*-------------------------------------------------------------
 * Loads values into the current matrix from the given array.
 * Matrix affinity is preserved if an affine matrix is loaded.
 *-------------------------------------------------------------*/

VG_API_CALL void vgLoadMatrix(const VGfloat * mm)
{
  SHMatrix3x3 *m;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  VG_RETURN_ERR_IF(!mm, VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);
  /* TODO: check matrix array alignment */
  
  m = shCurrentMatrix(context);

  if (context->matrixMode == VG_MATRIX_IMAGE_USER_TO_SURFACE) {
    
    SETMAT((*m),
           mm[0], mm[3], mm[6],
           mm[1], mm[4], mm[7],
           mm[2], mm[5], mm[8]);
  }else{
    
    SETMAT((*m),
           mm[0], mm[3], mm[6],
           mm[1], mm[4], mm[7],
           0.0f,  0.0f,  1.0f);
  }
  
  VG_RETURN(VG_NO_RETVAL);
}

/*---------------------------------------------------------------
 * Outputs the values of the current matrix into the given array
 *---------------------------------------------------------------*/

VG_API_CALL void vgGetMatrix(VGfloat * mm)
{
  SHMatrix3x3 *m; int i,j,k=0;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  VG_RETURN_ERR_IF(!mm, VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);
  /* TODO: check matrix array alignment */
  
  m = shCurrentMatrix(context);
  
  for (i=0; i<3; ++i)
    for (j=0; j<3; ++j)
      mm[k++] = m->m[j][i];
  
  VG_RETURN(VG_NO_RETVAL);
}

/*-------------------------------------------------------------
 * Right-multiplies the current matrix with the one specified
 * in the given array. Matrix affinity is preserved if an
 * affine matrix is begin multiplied.
 *-------------------------------------------------------------*/

VG_API_CALL void vgMultMatrix(const VGfloat * mm)
{
  SHMatrix3x3 *m, mul, temp;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  VG_RETURN_ERR_IF(!mm, VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);
  /* TODO: check matrix array alignment */
  
  m = shCurrentMatrix(context);
  
  if (context->matrixMode == VG_MATRIX_IMAGE_USER_TO_SURFACE) {
    
    SETMAT(mul,
           mm[0], mm[3], mm[6],
           mm[1], mm[4], mm[7],
           mm[2], mm[5], mm[8]);
  }else{
    
    SETMAT(mul,
           mm[0], mm[3], mm[6],
           mm[1], mm[4], mm[7],
           0.0f,  0.0f,  1.0f);
  }
  
  MULMATMAT((*m), mul, temp);
  SETMATMAT((*m), temp);
  
  VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void vgTranslate(VGfloat tx, VGfloat ty)
{
  SHMatrix3x3 *m;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  m = shCurrentMatrix(context);
  TRANSLATEMATR((*m), tx, ty);
  
  VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void vgScale(VGfloat sx, VGfloat sy)
{
  SHMatrix3x3 *m;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  m = shCurrentMatrix(context);
  SCALEMATR((*m), sx, sy);
  
  VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void vgShear(VGfloat shx, VGfloat shy)
{
  SHMatrix3x3 *m;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  m = shCurrentMatrix(context);
  SHEARMATR((*m), shx, shy);
  
  VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void vgRotate(VGfloat angle)
{
  SHfloat a;
  SHMatrix3x3 *m;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  a = SH_DEG2RAD(angle);
  m = shCurrentMatrix(context);
  ROTATEMATR((*m), a);
  
  VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL VGHardwareQueryResult vgHardwareQuery(VGHardwareQueryType key,
                                                  VGint setting)
{
  return VG_HARDWARE_UNACCELERATED;
}

void shBuildScissorContext(VGContext* c, SHint count, const void* values,
                           SHint floats)
{
  SHint numRects;
  SHVector3* vert;
  SHuint16* idx;
  SHuint16 rectIdx;
  SHint i;

  count = SH_MIN(count, SH_MAX_SCISSOR_RECTS * 4);
  numRects  = count / 4;
  shVector3ArrayReserve(&c->scissor, count);
  c->scissor.size = count;
  shUint16ArrayReserve(&c->scissorIndices, numRects * 6);
  c->scissorIndices.size = numRects * 6;
  for (vert = c->scissor.items, idx = c->scissorIndices.items, rectIdx = 0;
       rectIdx < count;
       vert += 4, idx += 6, rectIdx += 4) {
    SHRectangle r;
    r.x = shParamToFloat(values, floats, rectIdx + 0);
    r.y = shParamToFloat(values, floats, rectIdx + 1);
    r.w = shParamToFloat(values, floats, rectIdx + 2);
    r.h = shParamToFloat(values, floats, rectIdx + 3);
    if (r.w < 0 || r.h < 0) {
      r.w = 0;
      r.h = 0;
    }
    makeRectangle(r.x, r.y, r.w, r.h, -.5f, vert, idx, rectIdx);
  }
  if (c->scissoring == VG_TRUE)
    shEnableScissoring(c);
}

int shCopyOutScissorParams(VGContext *c, SHint count, void *values,
                           SHint floats)
{
  int i;
  
  if (count > (c->scissor.size))
    return 1;
  for (i = 0; i < c->scissor.size; i += 4) {
    SHRectangle r;
    
    r.x = c->scissor.items[i].x;
    r.y = c->scissor.items[i].y;
    r.w = c->scissor.items[i + 2].x - r.x;
    r.h = c->scissor.items[i + 2].y - r.y;
    /* XXX should round to nearest integer */
    shIntToParam((SHint)r.x, count, values, floats, i + 0);
    shIntToParam((SHint)r.y, count, values, floats, i + 1);
    shIntToParam((SHint)r.w, count, values, floats, i + 2);
    shIntToParam((SHint)r.h, count, values, floats, i + 3);
  }
  return 0;
}

/* The scissor test makes use of the depth buffer. The default ortho2D
   z range is from 1 to -1. The depth buffer is cleared to the near
   plane, so that nothing can pass the depth test. The scissor
   rectangles are drawn at -.5, which punches out a mask in the depth buffer.
   Stroke polygons and everything else are rendered at z = 0, so they
   will be rendered where the scissor rectangles were drawn. */

void shEnableScissoring(VGContext *c)
{
  glDepthMask(GL_TRUE);
  glClear(GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  if (c->scissor.size > 0) {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    /* Render scissor polys with depth test "disabled." */
    glDepthFunc(GL_ALWAYS);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, c->scissor.items);
    glDrawElements(GL_TRIANGLES, c->scissorIndices.size, GL_UNSIGNED_SHORT,
                   c->scissorIndices.items);
    glDisableClientState(GL_VERTEX_ARRAY);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  }
  glDepthMask(GL_FALSE);
  glDepthFunc(GL_LESS);
}

void shDisableScissoring(VGContext *c)
{
  glDisable(GL_DEPTH_TEST);
}
