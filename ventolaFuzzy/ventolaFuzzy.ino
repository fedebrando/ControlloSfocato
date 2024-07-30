
#include <Fuzzy.h>

#define PIN_TEMP A0
#define PIN_ENG 3

Fuzzy *ctrl;

void setup()
{
  Serial.begin(9600);

  // Fuzzy control system instance
  ctrl = new Fuzzy();

  // Input: temperature (Celsius)
  FuzzyInput *temp = new FuzzyInput(0);
  FuzzySet *tempLow = new FuzzySet(0, 5, 10, 15);
  FuzzySet *tempMedium = new FuzzySet(10, 15, 20, 25);
  FuzzySet *tempHigh = new FuzzySet(20, 25, 30, 35);
  temp->addFuzzySet(tempLow);
  temp->addFuzzySet(tempMedium);
  temp->addFuzzySet(tempHigh);
  ctrl->addFuzzyInput(temp);

  // Output: engine speed (rpm)
  FuzzyOutput *eng = new FuzzyOutput(0);
  FuzzySet *engSlow = new FuzzySet(500, 1500, 2000, 3000);
  FuzzySet *engModerate = new FuzzySet(2000, 3000, 3500, 4500);
  FuzzySet *engFast = new FuzzySet(3500, 4500, 5000, 6000);
  eng->addFuzzySet(engSlow);
  eng->addFuzzySet(engModerate);
  eng->addFuzzySet(engFast);
  ctrl->addFuzzyOutput(eng);

  // Fuzzy rules
  FuzzyRuleAntecedent *ifTempLow = new FuzzyRuleAntecedent();
  ifTempLow->joinSingle(tempLow);
  FuzzyRuleConsequent *thenEngSlow = new FuzzyRuleConsequent();
  thenEngSlow->addOutput(engSlow);
  FuzzyRule *rule1 = new FuzzyRule(0, ifTempLow, thenEngSlow);
  ctrl->addFuzzyRule(rule1);

  FuzzyRuleAntecedent *ifTempMedium = new FuzzyRuleAntecedent();
  ifTempMedium->joinSingle(tempMedium);
  FuzzyRuleConsequent *thenEngModerate = new FuzzyRuleConsequent();
  thenEngModerate->addOutput(engModerate);
  FuzzyRule *rule2 = new FuzzyRule(1, ifTempMedium, thenEngModerate);
  ctrl->addFuzzyRule(rule2);

  FuzzyRuleAntecedent *ifTempHigh = new FuzzyRuleAntecedent();
  ifTempHigh->joinSingle(tempHigh);
  FuzzyRuleConsequent *thenEngFast = new FuzzyRuleConsequent();
  thenEngFast->addOutput(engFast);
  FuzzyRule *rule3 = new FuzzyRule(2, ifTempHigh, thenEngFast);
  ctrl->addFuzzyRule(rule3);

  // Pin
  pinMode(PIN_ENG, OUTPUT);
}

float mapFloat(int val, int a1, int b1, float a2, float b2)
{
  return ((float)(val - a1) / (b1 - a1)) * (b2 - a2) + a2;
}

void loop()
{
  int power, pwmValue;
  float temp, eng_speed;

  // Input
  power = analogRead(PIN_TEMP);
  temp = mapFloat(power, 0, 1023, 0, 50); // virtual temperature value (power is a 2-Byte integer)

  // Fuzzy control system
  ctrl->setInput(0, temp); // input crisp
  ctrl->fuzzify();
  eng_speed = ctrl->defuzzify(0); // output crisp

  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print(" -> Speed: ");
  Serial.println(eng_speed);

  // Output
  pwmValue = map(eng_speed, 0, 6000, 0, 255); // conversion from virtual speed to pwm value
  analogWrite(PIN_ENG, pwmValue);

  delay(1000);
}
