#include <Arduino.h>

enum EtatCible
{
  FERME,
  PENDANT,
  OUVERT
};

class Porte
{

public:
  Porte(const int stepperMotorPins[4], const int * closedDetectPin, const int * openedDetectPin)
  {
    _stepperMotorPins = stepperMotorPins;
    _closedDetectPin = closedDetectPin;
    _openedDetectPin = openedDetectPin;
  }

  void ouvrir()
  {
    Porte::etatCible = OUVERT;
    _moveAngle(false, 90, 3);
  }

  void fermer()
  {
    Porte::etatCible = FERME;
    _moveAngle(true, 90, 3);
  }

  int getEtat()
  {
    return Porte::etatActuel;
  }

  void loop()
  {

  }

private:
  int etatActuel;
  EtatCible etatCible;
  //DateTime lastUpdateTS;

  const int *_stepperMotorPins;
  const int *_closedDetectPin;
  const int *_openedDetectPin;

  bool est_ferme() const noexcept
  {
    return etatActuel == FERME;
  }
  bool est_ouvert() const noexcept
  {
    return etatActuel == OUVERT;
  }



  //Suggestion: the motor turns precisely when the ms range is between 3 and 20
  void _moveSteps(bool dir, int steps, byte ms)
  {
    for (unsigned long i = 0; i < steps; i++)
    {
      _moveOneStep(dir);            // Rotate a step
      delay(constrain(ms, 3, 20)); // Control the speed
    }
  }

  void _moveOneStep(bool dir)
  {
    // Define a variable, use four low bit to indicate the state of port
    static byte out = 0x01;
    // Decide the shift direction according to the rotation direction
    if (dir)
    {
      // ring shift left
      out != 0x08 ? out = out << 1 : out = 0x01;
    }
    else
    {
      // ring shift right
      out != 0x01 ? out = out >> 1 : out = 0x08;
    }
    // Output singal to each port
    for (int i = 0; i < 4; i++)
    {
      digitalWrite(_stepperMotorPins[i], (out & (0x01 << i)) ? HIGH : LOW);
    }
  }

  void _moveAround(bool dir, int turns, byte ms)
  {
    for (int i = 0; i < turns; i++)
      _moveSteps(dir, 32 * 64, ms);
  }

  void _moveAngle(bool dir, int angle, byte ms)
  {
    _moveSteps(dir, (angle * 32 * 64 / 360), ms);
  }
};
