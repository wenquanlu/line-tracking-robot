import processing.serial.*;

Serial myPort;  // Create object from Serial class

void setup() 
{
  frameRate(10);
  size(200,200); //make our canvas 200 x 200 pixels big
  String portName = Serial.list()[1]; //change the 0 to a 1 or 2 etc. to match your port
  myPort = new Serial(this, portName, 9600);
}
void draw() {
  if (keyPressed == true) {
    if (key == 'w' || key == 'W') {
      myPort.write('w');
    }
    if (key == 's' || key == 'S') {
      myPort.write('s');
    }
    if (key == 'a' || key == 'A') {
      myPort.write('a');
    }
    if (key == 'd' || key == 'D') {
      myPort.write('d');
    }
    if (key == 'c' || key == 'C') {
      myPort.write('c');
    }
    if (key == 'p' || key == 'P') {
      myPort.write('p');
    }
    if (key == 'h' || key == 'H' ) {
      myPort.write('h');
    }
  } 
}
