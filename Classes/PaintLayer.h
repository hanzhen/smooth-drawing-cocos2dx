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


#include "cocos2d.h"

USING_NS_CC;

typedef struct _LineVertex {
    CCPoint pos;
    float z;
    ccColor4F color;
} LineVertex;

class LinePoint : public CCObject
{
public:
    CCPoint pos;
    float width;
    
    LinePoint();
    virtual ~LinePoint();
};

class PaintLayer : public CCLayer
{
private:
    void fillLineTriangles(LineVertex *vertices, int count, ccColor4F color);
    void startNewLineFrom(CCPoint newPoint, float aSize);
    void endLineAt(CCPoint aEndPoint, float aSize);
    void addPoint(CCPoint newPoint, float size);
    void drawLines(CCArray *linePoints, ccColor4F color);
    void fillLineEndPointAt(CCPoint center, CCPoint aLineDir, float radius, ccColor4F color);
    CCArray *calculateSmoothLinePoints();
    
public:
    virtual bool init();
    CREATE_FUNC(PaintLayer);
    static cocos2d::CCScene* scene();
    
    PaintLayer();
    virtual ~PaintLayer();
    
    virtual void draw(void);
    
    CCArray *points;
    CCArray *velocities;
    CCArray *circlesPoints;
    
    bool connectingLine;
    CCPoint prevC;
    CCPoint prevD;
    CCPoint prevG;
    CCPoint prevI;
    float overdraw;
    float lineWidth;
    
    CCRenderTexture *renderTexture;
    bool finishingLine;
    
    virtual bool ccTouchBegan(CCTouch* touch, CCEvent* event);
    virtual void ccTouchMoved(CCTouch* touch, CCEvent* event);
    virtual void ccTouchEnded(CCTouch* touch, CCEvent* event);
    
    virtual void onEnter();
    virtual void onEnterTransitionDidFinish();
    virtual void onExit();
};
