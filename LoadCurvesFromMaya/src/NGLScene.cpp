#include <QMouseEvent>
#include <QGuiApplication>
#include <QTextStream>
#include <QFile>
#include "NGLScene.h"
#include <ngl/Transformation.h>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>


NGLScene::NGLScene()
{
  // re-size the widget to that of the parent (in this case the GLFrame passed in on construction)
  setTitle("Using ngl::BezierCurve");
}


NGLScene::~NGLScene()
{
  std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";
}

void NGLScene::resizeGL( int _w, int _h )
{
  m_project=ngl::perspective( 45.0f, static_cast<float>( _w ) / _h, 0.05f, 350.0f );
  m_win.width  = static_cast<int>( _w * devicePixelRatio() );
  m_win.height = static_cast<int>( _h * devicePixelRatio() );
}

void NGLScene::initializeGL()
{
  // we must call this first before any other GL commands to load and link the
  // gl commands from the lib, if this is not done program will crash
  ngl::NGLInit::initialize();

  glClearColor(0.4f, 0.4f, 0.4f, 1.0f);			   // Grey Background
  // enable depth testing for drawing
  glEnable(GL_DEPTH_TEST);
  // enable multisampling for smoother drawing
  glEnable(GL_MULTISAMPLE);
  // Now we will create a basic Camera from the graphics library
  // This is a static camera so it only needs to be set once
  // First create Values for the camera position
  ngl::Vec3 from(0.0f,1.0f,15.0f);
  ngl::Vec3 to(0.0f,0.0f,0.0f);
  ngl::Vec3 up(0.0f,1.0f,0.0f);
  m_view=ngl::lookAt(from,to,up);
  m_project=ngl::perspective(45.0f,720.0f/576.0f,0.5f,150.0f);
  
  ngl::ShaderLib::use(ngl::nglColourShader);
  ngl::ShaderLib::setUniform("Colour",1.0f,1.0f,1.0f,1.0f);
  loadCurves();
}

void NGLScene::loadCurves()
{
  QFile file("loads.curve");
   QString line;
   int index=-1;
   if ( file.open(QIODevice::ReadOnly ) )
   {
     // file opened successfully
     QTextStream t( &file );        // use a text stream
     // until end of file...
     while ( !t.atEnd())
     {

       line = t.readLine();
       //std::cout<<"line "<<line.toStdString()<<'\n';
       QStringList lines=line.split(" ");
       if(lines[0]=="Curve")
       {
         std::cout<<"got curve \n";
         m_curves.push_back(std::unique_ptr<ngl::BezierCurve>(new ngl::BezierCurve()));
         ++index;
       }
       else if(lines[0]=="P")
       {
         std::cout<<"adding at index"<<index<<'\n';
         std::cout<<"p "<<lines[1].toFloat()<<" "<<lines[2].toFloat()<<" "<<lines[3].toFloat()<<'\n';
         m_curves[index]->addPoint(lines[1].toFloat(),lines[2].toFloat(),lines[3].toFloat());
       }
       // line of text excluding '\n'
     }
     // Close the file
     file.close();
   }
   std::cout<<"done file read\n";
   for(auto &p : m_curves)
   {
     p->setLOD(200);
     p->createKnots();
     p->createVAO();
   }
}


void NGLScene::loadMatricesToShader()
{
  ngl::ShaderLib::use(ngl::nglColourShader);
  ngl::Mat4 MVP;
  MVP= m_project * m_view * m_mouseGlobalTX;
  ngl::ShaderLib::setUniform("MVP",MVP);
 }

void NGLScene::paintGL()
{
  // clear the screen and depth buffer
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   // Rotation based on the mouse position for our global transform
   ngl::Mat4 rotX;
   ngl::Mat4 rotY;
   // create the rotation matrices
   rotX.rotateX(m_win.spinXFace);
   rotY.rotateY(m_win.spinYFace);
   // multiply the rotations
   m_mouseGlobalTX=rotY*rotX;
   // add the translations
   m_mouseGlobalTX.m_m[3][0] = m_modelPos.m_x;
   m_mouseGlobalTX.m_m[3][1] = m_modelPos.m_y;
   m_mouseGlobalTX.m_m[3][2] = m_modelPos.m_z;
   loadMatricesToShader();

   ngl::ShaderLib::use("nglColourShader");


   for(auto &c : m_curves)
   {
     ngl::ShaderLib::setUniform("Colour",1.0f,1.0f,1.0f,1.0f);
     c->draw();
     glPointSize(4);
     ngl::ShaderLib::setUniform("Colour",0.0f,1.0f,0.0f,1.0f);

     c->drawControlPoints();
     glPointSize(1);
     ngl::ShaderLib::setUniform("Colour",1.0f,0.0f,0.0f,1.0f);

     c->drawHull();

   }
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseMoveEvent( QMouseEvent* _event )
{
  // note the method buttons() is the button state when event was called
  // that is different from button() which is used to check which button was
  // pressed when the mousePress/Release event is generated
  if ( m_win.rotate && _event->buttons() == Qt::LeftButton )
  {
    int diffx = _event->x() - m_win.origX;
    int diffy = _event->y() - m_win.origY;
    m_win.spinXFace += static_cast<int>( 0.5f * diffy );
    m_win.spinYFace += static_cast<int>( 0.5f * diffx );
    m_win.origX = _event->x();
    m_win.origY = _event->y();
    update();
  }
  // right mouse translate code
  else if ( m_win.translate && _event->buttons() == Qt::RightButton )
  {
    int diffX      = static_cast<int>( _event->x() - m_win.origXPos );
    int diffY      = static_cast<int>( _event->y() - m_win.origYPos );
    m_win.origXPos = _event->x();
    m_win.origYPos = _event->y();
    m_modelPos.m_x += INCREMENT * diffX;
    m_modelPos.m_y -= INCREMENT * diffY;
    update();
  }
}


//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mousePressEvent( QMouseEvent* _event )
{
  // that method is called when the mouse button is pressed in this case we
  // store the value where the maouse was clicked (x,y) and set the Rotate flag to true
  if ( _event->button() == Qt::LeftButton )
  {
    m_win.origX  = _event->x();
    m_win.origY  = _event->y();
    m_win.rotate = true;
  }
  // right mouse translate mode
  else if ( _event->button() == Qt::RightButton )
  {
    m_win.origXPos  = _event->x();
    m_win.origYPos  = _event->y();
    m_win.translate = true;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseReleaseEvent( QMouseEvent* _event )
{
  // that event is called when the mouse button is released
  // we then set Rotate to false
  if ( _event->button() == Qt::LeftButton )
  {
    m_win.rotate = false;
  }
  // right mouse translate mode
  if ( _event->button() == Qt::RightButton )
  {
    m_win.translate = false;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::wheelEvent( QWheelEvent* _event )
{

  // check the diff of the wheel position (0 means no change)
  if ( _event->angleDelta().x() > 0 )
  {
    m_modelPos.m_z += ZOOM;
  }
  else if ( _event->angleDelta().x() < 0 )
  {
    m_modelPos.m_z -= ZOOM;
  }
  update();
}


//----------------------------------------------------------------------------------------------------------------------

void NGLScene::keyPressEvent(QKeyEvent *_event)
{
  // this method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  switch (_event->key())
  {
  // escape key to quite
  case Qt::Key_Escape : QGuiApplication::exit(EXIT_SUCCESS); break;
  // turn on wirframe rendering
  case Qt::Key_W : glPolygonMode(GL_FRONT_AND_BACK,GL_LINE); break;
  // turn off wire frame
  case Qt::Key_S : glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); break;
  // show full screen
  case Qt::Key_F : showFullScreen(); break;
  // show windowed
  case Qt::Key_N : showNormal(); break;
  default : break;
  }
  // finally update the GLWindow and re-draw
  update();
}
