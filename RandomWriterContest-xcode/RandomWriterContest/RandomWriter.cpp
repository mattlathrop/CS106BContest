/*
 * File: RandomWriter.cpp
 * ----------------------
 * Name: Matt Lathrop
 * Section: Aaron Acosta
 * Fun program that allows users to play with graphical recusion with shapes they draw.
 */

#include <fstream>
#include <iostream>
#include <string>
#include "console.h"
#include "map.h"
#include "random.h"
#include "strlib.h"
#include "vector.h"
#include "gwindow.h"
#include "ginteractors.h"
#include "gevents.h"
#include <math.h>
#include "gmath.h"
#include "simpio.h"
using namespace std;

/* Prototyopes */
void programLoop(Vector<GCompound*> &shapes, GWindow &gw, bool instructions);
void initGraphics(GWindow &gw, Vector<GCompound*> shapes);
GPolygon flipPolygon(GPolygon &baseShape);
GPolygon duplicatePolygon(GPolygon &baseShape);
Vector<GPolygon*> nextOrder(GWindow &gw, Vector<GPolygon*> outerShapes, int order, int color, double scale, Vector<GPolygon*> &objects, bool back, bool multi);
int colorFinder();

/* Interactors */
GLabel *orderLabel;
GTextField *orderField;
GSlider *red;
GSlider *green;
GSlider *blue;
GSlider *scaleSlider;
GButton *finish;
GSlider *zoom;
GLabel *zoomLabel;
GRect *colorViewer;
GButton *nextFigure;
GChooser *zChooser;
GButton *clear;
GCheckBox *multiply;

int main() {
    // init graphics interactors
    orderLabel = new GLabel("Orders to Advance");
    orderField = new GTextField();
    red = new GSlider(0, 0xFF, 0);
    green = new GSlider(0, 0xFF, 0);
    blue = new GSlider(0, 0xFF, 0);
    scaleSlider = new GSlider(1, 100, 100);
    zoom = new GSlider(1,200,50);
    finish = new GButton("Finish");
    zoomLabel = new GLabel("Zoom");
    nextFigure = new GButton("Next Figure");
    zChooser = new GChooser();
    zChooser->addItem("Front");
    zChooser->addItem("Back");
    clear = new GButton("Clear");
    multiply = new GCheckBox("Multiply");
    
    GWindow gw(1024,768); //Apple 27in 2560,1300 -- Apple Retina Mac 1680,900 (if scaled) -- other (1024,768) is safe.
    
    Vector<GCompound*> shapes; // Stores all the FINISHED shapes on the screen for re-drawing. 
    initGraphics(gw, shapes);
    //string instructionsToggle = getLine("Do you want instructions? (Yes or No): ");
    string instructionsToggle = "no";
    //cout << "You may close this window" << endl;
    bool instructions = false;
    if (equalsIgnoreCase(instructionsToggle, "yes")) {
        instructions = true;
    }
    while(true) {
        programLoop(shapes, gw, instructions);
        instructions = false;
    }
	return 0;
}

/* Adds the basic interactors to the screen and initializes labels. Also adds any previosuly drawn shapes */

void initGraphics(GWindow &gw, Vector<GCompound*> shapes) {
    
    // Sets all sliders to generate an action command.
    orderField->setActionCommand("order");
    red->setActionCommand("red");
    green->setActionCommand("green");
    blue->setActionCommand("blue");
    scaleSlider->setActionCommand("scale");
    
    //Initializes labels.
    GLabel *redLabel = new GLabel("Red");
    GLabel *greenLabel = new GLabel("Green");
    GLabel *blueLabel = new GLabel("Blue");
    GLabel *scaleLabel = new GLabel("Scale");
    GLabel *zChooserLabel = new GLabel("Add new to Front or Back?");
    
    // adds interactors to screen
    gw.addToRegion(orderLabel, "EAST");
    gw.addToRegion(orderField, "EAST");
    gw.addToRegion(zChooserLabel, "EAST");
    gw.addToRegion(zChooser, "EAST");
    gw.addToRegion(multiply, "EAST");
    gw.addToRegion(redLabel, "EAST");
    gw.addToRegion(red, "EAST");
    gw.addToRegion(greenLabel, "EAST");
    gw.addToRegion(green, "EAST");
    gw.addToRegion(blueLabel, "EAST");
    gw.addToRegion(blue, "EAST");
    gw.addToRegion(scaleLabel, "EAST");
    gw.addToRegion(scaleSlider, "EAST");
    gw.addToRegion(finish, "EAST");
    gw.addToRegion(nextFigure, "SOUTH");
    gw.addToRegion(clear, "SOUTH");
    
    
    // adds the color viewer to the screen
    colorViewer = new GRect(30,30);
    colorViewer->setFilled(true);
    colorViewer->setFillColor(colorFinder());
    gw.add(colorViewer);
    
    // adds previsously generated shapes to the screen
    foreach(GCompound* shape in shapes) {
        gw.add(shape);
    }
}

/* This function contains all the code for running the program and instructions for waht interactors do at each stage. 
 * There are 3 stages:
 *  1. Create and object. 
 *      Here the user can change the color of the base shape and draw the base shape. The user is to click to add each
 *      side of the shape but should NOT click to add either the last side or the second to last. Hit Enter instead.
 *      In other words the user should position the mouse where they want the last vertex to be before the figure closes
 *      itself and then hit enter to submit the last vertex and draw in the closing side. The program will then mirror the
 *      image the user created and store the half of the shape that the user drew as the shape to recurse on.
 *  2. Adding recusive layers.
 *      The user can now add as many recusive orders as they would like. They can change color, scale, if it should
 *      be placed behind all the current layers or in front of, and whether it should center one smaller shape or add multiple
 *      figures to fill the gap. A new layer is added by enteting a integer into the text box
 *      (that represents the number of orders they would like to advance in that currently set style) and press enter. When
 *      the user is done adding layers they should hit finish to enter the final stage.
 *  3. Moveing and scaling.
 *      Now they shape is "finalized" and rendered into a GCompound. it is UNEDITABLE in this state. The user can now use the
 *      the mouse to move the shape around and the new "zoom" slider to scale the figure. The user can repeat these steps 
 *      for additional figures by clicking next figure which will submit the current figure in it's place.
 */

void programLoop(Vector<GCompound*> &shapes, GWindow &gw, bool instructions) {
    Vector<GPoint> points; // used for storeing the mouse clicks to eventually construct the polygon from lines
    //used to modify contol loop function based on user intent.
    bool active = false;
    bool afterClick = false;
    
    GPolygon baseShape; // where the shape to be recursed on is stored.
    GLine *line; // the active line that allows for more intuitive UI
    GPoint currrentMouseLocation; //used in loop
    
    //used if the user wants instructions. must be created reguardless to advoid errors.
    double height = gw.getHeight();
    double width = gw.getWidth();
    GRect *iBack = new GRect(0, height-70, width, 30);
    iBack->setFilled(true);
    iBack->setFillColor("BLACK");
    GLabel *iLabel = new GLabel("Click to draw the seed figure. Draw an open-ended figure. After you draw your last line, hit ENTER (do not click).");
    iLabel->setColor("WHITE");
    iLabel->setFont("Dialog-20");
    if (instructions) {
        gw.add(iBack);
        gw.add(iLabel, 0, height-45);
    }
    
    // Control Loop for stage one. Used for creating initial figure.
    while (true) {
        GEvent e = waitForEvent();
        Vector<GLine> name;
        if (e.getEventType() == MOUSE_PRESSED) {
            GMouseEvent m = e;
            GPoint point(m.getX(), m.getY());
            points.add(point);
            if (!active) active = true;
            baseShape.addVertex(m.getX(), m.getY());
            afterClick = true;
            if (instructions) {
                iLabel->setLabel("After you hit ENTER, the final line will be submitted and last side will be drawn.");
                gw.remove(iLabel);
                gw.add(iLabel);
            }
        }
        if (e.getEventType() == MOUSE_MOVED && active) {
            GMouseEvent m = e;
            currrentMouseLocation = GPoint(m.getX(), m.getY());
            if (active && !afterClick) {
                gw.remove(line);
                delete line;
            }
            if (active) {
                GPoint last = points[points.size() -1];
                line = new GLine(last.getX(), last.getY(), m.getX(), m.getY());
                gw.add(line);
                afterClick = false;
            }
        }
        // submits final base shape.
        if (e.getEventType() == KEY_TYPED) {
            GKeyEvent k = e;
            if ( k.getKeyChar() == '\12') {
                GMouseEvent m = e;
                baseShape.addVertex(currrentMouseLocation.getX(), currrentMouseLocation.getY());
                break;
            }
        }
        // updates color viewer
        if (e.getEventType() == ACTION_PERFORMED) {
            GActionEvent a = e;
            if (a.getActionCommand() == "red" || a.getActionCommand() == "green" || a.getActionCommand() == "blue") {
                gw.remove(colorViewer);
                colorViewer->setFillColor(colorFinder());
                gw.add(colorViewer);
            }
        }
    }
    gw.clear(); // clears the window because old lines no longer have pointers and can't be acessed. 
    initGraphics(gw, shapes); //Re-creates the interactors and UI
    Vector<GPolygon*> objects; // Used to store ALL shapes that go into making one figure.
    // Sets info for baseShape.
    baseShape.setFilled(true);
    baseShape.setFillColor(colorFinder());
    baseShape.setColor(colorFinder());
    gw.add(&baseShape);
    objects.add(&baseShape);
    // creates a flipped copy of base Shapee
    GPolygon flippedBaseShape = flipPolygon(baseShape);
    flippedBaseShape.setFilled(true);
    flippedBaseShape.setFillColor(colorFinder());
    flippedBaseShape.setColor(colorFinder());
    gw.add(&flippedBaseShape);
    objects.add(&flippedBaseShape);
    
    // creates the vector to store which shapes are on the outer layer of the shape and therfore need to be recursed on
    Vector<GPolygon*> outerShapes;
    outerShapes.add(&baseShape);
    outerShapes.add(&flippedBaseShape);
    
    GPoint mouseP;
    bool moving = false;
    if (instructions) {
        gw.add(iBack);
        iLabel->setLabel("Now adjust the color sliders to pick the color of the next layer. You can view the color made in the upper LEFT corner.");
        gw.add(iLabel);
    }
    // used to modify instructions behavior based on what has already been said.
    bool orderDone = false;
    bool siderDone = false;
    
    // Initiates stage 2 of the program where users can add additional layers
    while(true) {
        GEvent e = waitForEvent();
        if (e.getEventType() == ACTION_PERFORMED) {
            GActionEvent a = e;
            if (a.getActionCommand() == "order") {
                string str = orderField->getText();
                int order = stringToInteger(str);
                int color = colorFinder();
                double scale = scaleSlider->getValue() / 100.0;
                if (multiply) {
                    scale = 1/scale;
                    scale = round(scale);
                    scale = 1/scale;
                }
                bool back; // stores where the user wants the new objects to appear front or back
                if (zChooser->getSelectedItem() == "Back") {
                   back = true; 
                } else {
                    back = false;
                }
                bool multi = false; // sets if the user has selected the other fractle mode: multi
                if (multiply->isSelected()) {
                    multi = true;
                }
                outerShapes = nextOrder(gw, outerShapes, order, color, scale, objects, back, multi); // Advances figure.
                // Controls for intructions
                if (instructions) {
                    iLabel->setLabel("Click Finish when you are done adding to the design.");
                    gw.remove(iLabel);
                    gw.add(iLabel);
                    iBack->sendToFront();
                    iLabel->sendToFront();
                }
                if (instructions && !orderDone) {
                    iLabel->setLabel("GREAT! Look how cool RECURSION is! Click to Advance!");
                    gw.remove(iLabel);
                    gw.add(iLabel);
                    waitForClick();
                    iLabel->setLabel("You can now continue to add new orders with different colors or scales. Enter the number of orders you want to add with the current settings and hit Enter.");
                    gw.remove(iLabel);
                    gw.add(iLabel);
                    orderDone = true;
                }
            }
            if (a.getActionCommand() == "Finish") break;
            // color viewer update
            if (a.getActionCommand() == "red" || a.getActionCommand() == "green" || a.getActionCommand() == "blue") {
                gw.remove(colorViewer);
                colorViewer->setFillColor(colorFinder());
                gw.add(colorViewer);
                if (instructions && !siderDone) {
                    iLabel->setLabel("You can also move the scale slider to effect the size of the next layer or change where the next layer will appear with the drop down box. Click to Advnace.");
                    gw.remove(iLabel);
                    gw.add(iLabel);
                    waitForClick();
                    iLabel->setLabel("Enter 1 in the ORDER text field and press enter to continue.");
                    gw.remove(iLabel);
                    gw.add(iLabel);
                    siderDone = true;
                }

            }
        }
    }
    // adds zoom interactor for stage 3
    gw.addToRegion(zoomLabel, "EAST");
    gw.addToRegion(zoom, "EAST");
    zoom->setActionCommand("zoom");
    
    // creates a GCompound from the current figure rendering it un-editable
    GCompound *finalShape = new GCompound;
    foreach(GPolygon* shape in objects) {
        GPolygon newShape = duplicatePolygon(*shape);
        finalShape->add(&newShape, newShape.getX(), newShape.getY());
        gw.remove(shape);
    }
    gw.add(finalShape);
    // more instructions
    if (instructions) {
        iLabel->setLabel("You can now scale and move your figure. Use the zoom slider to scale the figure and your mouse to move it around. Click Next Figure when you are done.");
        gw.remove(iLabel);
        gw.add(iLabel);
        iBack->sendToFront();
        iLabel->sendToFront();
    }
    double lastScale = 1; // used durring loop
    
    // Stage 3 control loop
    while (true) {
        GEvent e = waitForEvent();
        // used more moving figure around
        if (e.getEventType() == MOUSE_PRESSED) {
            GMouseEvent m = e;
            mouseP = GPoint(m.getX(), m.getY());
        }
        if (e.getEventType() == MOUSE_DRAGGED) {
            GMouseEvent m = e;
            GPoint location;
            finalShape->move(m.getX() - mouseP.getX(), m.getY()- mouseP.getY());
            moving = true;
            mouseP = GPoint(m.getX(), m.getY());
        }
        // used for zooming figure
        if (e.getEventType() == ACTION_PERFORMED) {
            GActionEvent a = e;
            if (a.getActionCommand() == "zoom") {
                gw.remove(finalShape);
                finalShape->scale(1/lastScale);
                double scaleFactor = zoom->getValue() / 50.0;
                finalShape->scale(scaleFactor);
                gw.add(finalShape);
                lastScale = scaleFactor;
                if (instructions) {
                    iBack->sendToFront();
                    iLabel->sendToFront();
                }
            }
            
            // still allows color mixing but are completley useless until next figure is clicked.
            if (a.getActionCommand() == "red" || a.getActionCommand() == "green" || a.getActionCommand() == "blue") {
                gw.remove(colorViewer);
                colorViewer->setFillColor(colorFinder());
                gw.add(colorViewer);
            }
            // breaks out of loop and starts program over
            if ((a.getActionCommand() == "Next Figure")) {
                shapes.add(finalShape);
                if (instructions) {
                    iLabel->setLabel("That's it! Now PLAY! Try changing the color of the initial shape! Or enable multiply mode (which changes how scale is interpreted) Click to Finish!");
                    gw.remove(iLabel);
                    gw.add(iLabel);
                    waitForClick();
                }

                break;
            }
            if(a.getActionCommand() == "Clear") {
                gw.clear();
                shapes.clear();
                initGraphics(gw, shapes);
                break;
            }
        }
    }
}

/* Used as a workaround meathod because a shape can't both be on the canvas and in a GCompound. Creates an
 * exact copy of the figure to add to GCompound */

GPolygon duplicatePolygon(GPolygon &baseShape) {
    GPolygon shape;
    Vector<GPoint> verticies = baseShape.getVertices();
    foreach(GPoint point in verticies) {
        shape.addVertex(point.getX(), point.getY());
    }
    shape.setColor(baseShape.getColor());
    shape.setFillColor(baseShape.getFillColor());
    shape.setFilled(baseShape.isFilled());
    return shape;
}

/* Creates the mirrored polygon during Stage 1. Takes in a GPolygon and returns another.*/

GPolygon flipPolygon(GPolygon &baseShape) {
    GPolygon shape;
    Vector<GPoint> verticies = baseShape.getVertices();
    GPoint orginPoint = verticies[0];
    GPoint finalPoint = verticies[verticies.size()-1];
    double filldx = finalPoint.getX() - orginPoint.getX();
    double filldy = finalPoint.getY() - orginPoint.getY();
    double fillAngle = toDegrees(atan2(filldy,filldx));
    shape.addVertex(verticies[0].getX(), verticies[0].getY());
    for (int i = 0; i < verticies.size()-1; i++) {
        GPoint pt1 = verticies[i];
        GPoint pt2 = verticies[i + 1];
        double dx = pt2.getX() - pt1.getX();
        double dy = pt2.getY() - pt1.getY();
        double angle = toDegrees(atan2(dy,dx));
        double r = sqrt((dx * dx) + (dy * dy));
        shape.addPolarEdge(r, angle - fillAngle*2);
    }
    return shape;
}

/* Used for calcualting the color based off the values of the RGB sliders */

int colorFinder() {
    int redValue;
    int greenValue;
    int blueValue;
    try {
        redValue = (red->getValue()) * 65536;
        greenValue = (green->getValue()) * 256;
        blueValue = (blue->getValue());
    } catch (...) {
        //nothing
    }
    int hex = redValue + greenValue + blueValue;
    return hex;
}


/* Main recursive backbone of the program. Does all the work for figureing out where new shapes need to go.
 * Very messy graphics math! Dare ye enter? Here be dragons!!!
 */

Vector<GPolygon*> nextOrder(GWindow &gw, Vector<GPolygon*> outerShapes, int order, int color, double scale, Vector<GPolygon*> &objects, bool back, bool multi) {
    if (order == 0) return outerShapes; // base case. if we have don all orders then return.
    Vector<GPolygon*> result; // stores the list of the outermost shapes.
    
    if (order !=0 ){
        foreach( GPolygon* outerShape in outerShapes) {
            // calculates information about the baseshape in relation to initial shape
            Vector<GPoint> verticies = outerShape->getVertices();
            GPoint orginPoint = verticies[0];
            GPoint finalPoint = verticies[verticies.size()-1];
            double filldx = finalPoint.getX() - orginPoint.getX();
            double filldy = finalPoint.getY() - orginPoint.getY();
            double fillAngle = toDegrees(atan2(filldy,filldx));
            double emptySideR = sqrt((filldx * filldx) + (filldy * filldy));
            int loopSize;
            loopSize = verticies.size()-1;
            // uses info to create other shapes around each side of each outer shape.
            for (int i = 0; i < loopSize; i++) {
                for (int k = 0; k < 1/scale; k++) {
                    GPolygon* newSide = new GPolygon;
                    GPoint pt1 = verticies[i];
                    GPoint pt2;
                    pt2 = verticies[i + 1];
                    double dx = pt2.getX() - pt1.getX();
                    double dy = pt2.getY() - pt1.getY();
                    double angle = toDegrees(atan2(dy,dx));
                    double modAngle = angle - fillAngle;
                    double r = sqrt((dx * dx) + (dy * dy));
                    double shapeScale = (r/emptySideR) * scale;
                    if (scale == 1) {
                        newSide->addVertex(verticies[i].getX(), verticies[i].getY());
                    } else if (multi) {
                        double x = (cosDegrees(angle) * scale * k * r) + verticies[i].getX();
                        double y = (sinDegrees(angle) * scale * k * r) + verticies[i].getY();
                        newSide->addVertex(x, y);
                    } else {
                        double x = (cosDegrees(angle) * ((1-scale)/2.0)* r) + verticies[i].getX();
                        double y = (sinDegrees(angle) * ((1-scale)/2.0)* r) + verticies[i].getY();
                        newSide->addVertex(x, y);
                    }
                    for (int j = 0; j < loopSize; j++) {
                        GPoint shapePt1 = verticies[j];
                        GPoint shapePt2;
                        shapePt2 = verticies[j + 1];
                        double shapedx = shapePt2.getX() - shapePt1.getX();
                        double shapedy = shapePt2.getY() - shapePt1.getY();
                        double shapeR = sqrt((shapedx *shapedx) + (shapedy*shapedy)) * shapeScale;
                        double shapeAngle = toDegrees(atan2(shapedy,shapedx));
                        newSide->addPolarEdge(shapeR, -(modAngle + (shapeAngle)));
                    }
                    result.add(newSide);
                    newSide->setFilled(true);
                    newSide->setFillColor(color);
                    gw.add(newSide);
                    //newSide->setColor(color);
                    if (back) {
                        objects.insert(0, newSide);
                        newSide->sendToBack();
                    } else {
                        objects.add(newSide);
                    }
                }
            }
        }
        result = nextOrder(gw, result, order-1, color, scale, objects, back, multi);
    }
    return result;
}
