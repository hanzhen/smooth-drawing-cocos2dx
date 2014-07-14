/*
 * Smooth drawing: http://merowing.info
 *
 * Copyright (c) 2012 Krzysztof Zabłocki
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#include "PaintLayer.h"

#define visibleSize         CCDirector::sharedDirector()->getVisibleSize()

LinePoint::LinePoint()
{
}

LinePoint::~LinePoint()
{
}

CCScene* PaintLayer::scene()
{
    CCScene* scene = CCScene::create();
    CCLayer* layer = PaintLayer::create();
    scene->addChild(layer);
    return scene;
}

PaintLayer::PaintLayer()
{
    lineWidth = 20.0;
    
    points = CCArray::create();
    velocities = CCArray::create();
    circlesPoints = CCArray::create();
    
    points->retain();
    velocities->retain();
    circlesPoints->retain();
}

PaintLayer::~PaintLayer()
{
    CC_SAFE_RELEASE(points);
    CC_SAFE_RELEASE(velocities);
    CC_SAFE_RELEASE(circlesPoints);
}

bool PaintLayer::init()
{
    bool bRet = false;
    do
    {
        CC_BREAK_IF(!CCLayer::init());
        
        setShaderProgram(CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionColor));
        overdraw = 3.0f;
        
        renderTexture = CCRenderTexture::create(visibleSize.width, visibleSize.height,kCCTexture2DPixelFormat_RGBA8888);
        renderTexture->setPosition(visibleSize.width/2, visibleSize.height/2);
        renderTexture->clear(1.0, 1.0, 1.0, 1.0);
        
        addChild(renderTexture);
        
        bRet = true;
    } while(0);
    
    return bRet;
}

#pragma mark - Handling points
void PaintLayer::startNewLineFrom(CCPoint newPoint, float aSize)
{
    connectingLine = false;
    addPoint(newPoint, aSize);
}

void PaintLayer::endLineAt(CCPoint aEndPoint, float aSize)
{
    addPoint(aEndPoint, aSize);
    finishingLine = true;
}

void PaintLayer::addPoint(CCPoint newPoint, float size)
{
    LinePoint *point = new LinePoint();
    point->pos = newPoint;
    point->width = size;
    point->autorelease();
    points->addObject(point);
}

#pragma mark - Drawing

#define ADD_TRIANGLE(A, B, C, Z) vertices[index].pos = A, vertices[index++].z = Z, vertices[index].pos = B, vertices[index++].z = Z, vertices[index].pos = C, vertices[index++].z = Z

void PaintLayer::drawLines(CCArray *linePoints, ccColor4F color)
{
    unsigned int numberOfVertices = (linePoints->count() - 1) * 18;
    LineVertex *vertices = (LineVertex *)calloc(sizeof(LineVertex), numberOfVertices);

    CCPoint prevPoint =  ((LinePoint *)linePoints->objectAtIndex(0))->pos;
    float prevValue = ((LinePoint *)linePoints->objectAtIndex(0))->width;
    float curValue;
    int index = 0;
    for (int i = 1; i < linePoints->count(); ++i)
    {
        LinePoint *pointValue = (LinePoint *)linePoints->objectAtIndex(i);
        CCPoint curPoint = pointValue->pos;
        curValue = pointValue->width;

        //! equal points, skip them
        if (ccpFuzzyEqual(curPoint, prevPoint, 0.0001f))
        {
            continue;
        }

        CCPoint dir = ccpSub(curPoint, prevPoint);
        CCPoint perpendicular = ccpNormalize(ccpPerp(dir));
        CCPoint A = ccpAdd(prevPoint, ccpMult(perpendicular, prevValue / 2));
        CCPoint B = ccpSub(prevPoint, ccpMult(perpendicular, prevValue / 2));
        CCPoint C = ccpAdd(curPoint, ccpMult(perpendicular, curValue / 2));
        CCPoint D = ccpSub(curPoint, ccpMult(perpendicular, curValue / 2));

        //! continuing line
        if (connectingLine || index > 0)
        {
            A = prevC;
            B = prevD;
        }
        else if (index == 0)
        {
            //! circle at start of line, revert direction
            circlesPoints->addObject(pointValue);
            circlesPoints->addObject(linePoints->objectAtIndex(i-1));
        }

        ADD_TRIANGLE(A, B, C, 1.0f);
        ADD_TRIANGLE(B, C, D, 1.0f);

        prevD = D;
        prevC = C;
        if (finishingLine && (i == linePoints->count() - 1))
        {
            circlesPoints->addObject(linePoints->objectAtIndex(i-1));
            circlesPoints->addObject(pointValue);
            finishingLine = false;
        }
        prevPoint = curPoint;
        prevValue = curValue;

        //! Add overdraw
        CCPoint F = ccpAdd(A, ccpMult(perpendicular, overdraw));
        CCPoint G = ccpAdd(C, ccpMult(perpendicular, overdraw));
        CCPoint H = ccpSub(B, ccpMult(perpendicular, overdraw));
        CCPoint I = ccpSub(D, ccpMult(perpendicular, overdraw));

        //! end vertices of last line are the start of this one, also for the overdraw
        if (connectingLine || index > 6)
        {
            F = prevG;
            H = prevI;
        }

        prevG = G;
        prevI = I;

        ADD_TRIANGLE(F, A, G, 2.0f);
        ADD_TRIANGLE(A, G, C, 2.0f);
        ADD_TRIANGLE(B, H, D, 2.0f);
        ADD_TRIANGLE(H, D, I, 2.0f);
    }

    this->fillLineTriangles(vertices, index, color);

    if (index > 0)
    {
        connectingLine = true;
    }

    free(vertices);
}

void PaintLayer::fillLineEndPointAt(CCPoint center, CCPoint aLineDir, float radius, ccColor4F color)
{
    int numberOfSegments = 32;
    LineVertex *vertices = (LineVertex *)malloc(sizeof(LineVertex) * numberOfSegments * 9);
    float anglePerSegment = (float)(M_PI / (numberOfSegments - 1));

    //! we need to cover M_PI from this, dot product of normalized vectors is equal to cos angle between them... and if you include rightVec dot you get to know the correct direction :)
    CCPoint perpendicular = ccpPerp(aLineDir);
    float angle = acosf(ccpDot(perpendicular, CCPointMake(0, 1)));
    float rightDot = ccpDot(perpendicular, CCPointMake(1, 0));
    if (rightDot < 0.0f)
    {
        angle *= -1;
    }

    CCPoint prevPoint = center;
    CCPoint prevDir = ccp(sinf(0), cosf(0));
    for (unsigned int i = 0; i < numberOfSegments; ++i)
    {
        CCPoint dir = ccp(sinf(angle), cosf(angle));
        CCPoint curPoint = ccp(center.x + radius * dir.x, center.y + radius * dir.y);
        vertices[i * 9 + 0].pos = center;
        vertices[i * 9 + 1].pos = prevPoint;
        vertices[i * 9 + 2].pos = curPoint;

        //! fill rest of vertex data
        for (unsigned int j = 0; j < 9; ++j)
        {
            vertices[i * 9 + j].z = j < 3 ? 1.0f : 2.0f;
            vertices[i * 9 + j].color = color;
        }

        //! add overdraw
        vertices[i * 9 + 3].pos = ccpAdd(prevPoint, ccpMult(prevDir, overdraw));
        vertices[i * 9 + 3].color.a = 0;
        vertices[i * 9 + 4].pos = prevPoint;
        vertices[i * 9 + 5].pos = ccpAdd(curPoint, ccpMult(dir, overdraw));
        vertices[i * 9 + 5].color.a = 0;

        vertices[i * 9 + 6].pos = prevPoint;
        vertices[i * 9 + 7].pos = curPoint;
        vertices[i * 9 + 8].pos = ccpAdd(curPoint, ccpMult(dir, overdraw));
        vertices[i * 9 + 8].color.a = 0;

        prevPoint = curPoint;
        prevDir = dir;
        angle += anglePerSegment;
    }

    glVertexAttribPointer(kCCVertexAttrib_Position, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), &vertices[0].pos);
    glVertexAttribPointer(kCCVertexAttrib_Color, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), &vertices[0].color);
    glDrawArrays(GL_TRIANGLES, 0, numberOfSegments * 9);

    free(vertices);
}

void PaintLayer::fillLineTriangles(LineVertex *vertices, int count, ccColor4F color)
{
    getShaderProgram()->use();
    getShaderProgram()->setUniformsForBuiltins();

    ccGLEnableVertexAttribs(kCCVertexAttribFlag_Position | kCCVertexAttribFlag_Color);

    ccColor4F fullColor = color;
    ccColor4F fadeOutColor = color;
    fadeOutColor.a = 0;

    for (int i = 0; i < count / 18; ++i)
    {
        for (int j = 0; j < 6; ++j)
        {
            vertices[i * 18 + j].color = color;
        }

        //! FAG
        vertices[i * 18 + 6].color = fadeOutColor;
        vertices[i * 18 + 7].color = fullColor;
        vertices[i * 18 + 8].color = fadeOutColor;

        //! AGD
        vertices[i * 18 + 9].color = fullColor;
        vertices[i * 18 + 10].color = fadeOutColor;
        vertices[i * 18 + 11].color = fullColor;

        //! BHC
        vertices[i * 18 + 12].color = fullColor;
        vertices[i * 18 + 13].color = fadeOutColor;
        vertices[i * 18 + 14].color = fullColor;

        //! HCI
        vertices[i * 18 + 15].color = fadeOutColor;
        vertices[i * 18 + 16].color = fullColor;
        vertices[i * 18 + 17].color = fadeOutColor;
    }

    glVertexAttribPointer(kCCVertexAttrib_Position, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), &vertices[0].pos);
    glVertexAttribPointer(kCCVertexAttrib_Color, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), &vertices[0].color);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)count);

    for (unsigned int i = 0; i < circlesPoints->count() / 2;   ++i)
    {
        LinePoint *prevPoint = (LinePoint *)circlesPoints->objectAtIndex(i * 2);
        LinePoint *curPoint = (LinePoint *)circlesPoints->objectAtIndex(i * 2 + 1);
        CCPoint dirVector = ccpNormalize(ccpSub(curPoint->pos, prevPoint->pos));

        this->fillLineEndPointAt(curPoint->pos, dirVector, curPoint->width * 0.4f, color);
    }
    circlesPoints->removeAllObjects();
}

CCArray *PaintLayer::calculateSmoothLinePoints()
{
    if (points->count() > 2)
    {
        CCArray *smoothedPoints = CCArray::create();
        for (unsigned int i = 2; i < points->count(); ++i)
        {
            LinePoint *prev2 = (LinePoint *)points->objectAtIndex(i - 2);
            LinePoint *prev1 = (LinePoint *)points->objectAtIndex(i - 1);
            LinePoint *cur = (LinePoint *)points->objectAtIndex(i);

            CCPoint midPoint1 = ccpMult(ccpAdd(prev1->pos, prev2->pos), 0.5f);
            CCPoint midPoint2 = ccpMult(ccpAdd(cur->pos, prev1->pos), 0.5f);

            int segmentDistance = 2;
            float distance = ccpDistance(midPoint1, midPoint2);
            int numberOfSegments = MIN(128, MAX(floorf(distance / segmentDistance), 32));

            float t = 0.0f;
            float step = 1.0f / numberOfSegments;
            for (int j = 0; j < numberOfSegments; j++)
            {
                LinePoint *newPoint = new LinePoint;
                newPoint->pos = ccpAdd(ccpAdd(ccpMult(midPoint1, powf(1 - t, 2)), ccpMult(prev1->pos, 2.0f * (1 - t) * t)), ccpMult(midPoint2, t * t));
                newPoint->width = powf(1 - t, 2) * ((prev1->width + prev2->width) * 0.5f) + 2.0f * (1 - t) * t * prev1->width + t * t * ((cur->width + prev1->width) * 0.5f);

                newPoint->autorelease();
                smoothedPoints->addObject(newPoint);
                t += step;
            }
            LinePoint *finalPoint = new LinePoint;
            finalPoint->pos = midPoint2;
            finalPoint->width = (cur->width + prev1->width) * 0.5f;
            finalPoint->autorelease();
            smoothedPoints->addObject(finalPoint);
        }
        
        //! we need to leave last 2 points for next draw
        while(points->count() > 2)
        {
            points->removeObjectAtIndex(0);
        }
        
        return smoothedPoints;
    }
    else
    {
        return NULL;
    }
}

void PaintLayer::draw(void)
{
    ccColor4F color = {0, 0, 1, 1};
    renderTexture->begin();

    CCArray *smoothedPoints = this->calculateSmoothLinePoints();
    if (smoothedPoints != NULL)
    {
        drawLines(smoothedPoints, color);
    }
    
    renderTexture->end();
}

bool PaintLayer::ccTouchBegan(CCTouch *touch, CCEvent *event)
{
    const CCPoint point = CCDirector::sharedDirector()->convertToGL(touch->getLocationInView());
    
    points->removeAllObjects();
    velocities->removeAllObjects();
    
    this->startNewLineFrom(point, lineWidth);
    
    this->addPoint(point, lineWidth);

    return true;
}

void PaintLayer::ccTouchMoved(CCTouch* touch, CCEvent* event)
{
    const CCPoint point = CCDirector::sharedDirector()->convertToGL(touch->getLocationInView());

    //! skip points that are too close
    float eps = 1.5;
    if (points->count() > 0)
    {
        float length = ccpLength(ccpSub(((LinePoint *)points->lastObject())->pos, point));

        if (length < eps)
        {
            return;
        }
    }
    this->addPoint(point, lineWidth);
}

void PaintLayer::ccTouchEnded(CCTouch* touch, CCEvent* event)
{
    const CCPoint point = CCDirector::sharedDirector()->convertToGL(touch->getLocationInView());
    
    this->endLineAt(point, lineWidth);
}

void PaintLayer::onEnter()
{
    CCLayer::onEnter();
}

void PaintLayer::onEnterTransitionDidFinish()
{
    CCLayer::onEnterTransitionDidFinish();
    CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, 0, true);
}

void PaintLayer::onExit()
{
    CCLayer::onExit();
}

