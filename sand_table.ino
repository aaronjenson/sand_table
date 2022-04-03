#include <AceSorting.h>

#include <AccelStepper.h>
#include <MultiStepper.h>
#include <SPI.h>
#include <SD.h>

#define STEPS_PER_REV 3821
#define STEPS_RHO     4529

#define MAX_FILES 10

// EG X-Y position bed driven by 2 steppers
// Alas its not possible to build an array of these with different pins for each :-(
AccelStepper theta_stepper(AccelStepper::FULL4WIRE, 43, 47, 45, 49);
AccelStepper rho_stepper(AccelStepper::FULL4WIRE, 42, 46, 44, 48);

// Up to 10 steppers can be handled as a group by MultiStepper
MultiStepper steppers;

File root;

String files[MAX_FILES];
int num_files = 0;
int curr_file = 0;

void setup() {
  // disable ethernet
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  Serial.begin(9600);
  // while(!Serial);

  Serial.print("Initializing SD card...");
  if (!SD.begin(4)) {
    Serial.println("failed!");
    while (1);
  }
  Serial.println("ok.");

  Serial.print("Opening base dir...");
  root = SD.open("/");
  if (!root) {
    Serial.println("failed!");
    while (1);
  }
  Serial.println("ok.");

  for (; num_files < MAX_FILES; num_files++) {
    File file = root.openNextFile();
    if (!file) break;
    if (!String(file.name()).endsWith(String(".THR"))) {
      num_files--;
      continue;
    }
    files[num_files] = file.name();
    file.close();
  }

  ace_sorting::shellSortKnuth(files, num_files);

  Serial.println("file order:");
  for (int i = 0; i < num_files; i++) {
    Serial.println(files[i]);
  }

  // Configure each stepper
  theta_stepper.setMaxSpeed(400);
  rho_stepper.setMaxSpeed(400);

  // Then give them to MultiStepper to manage
  steppers.addStepper(theta_stepper);
  steppers.addStepper(rho_stepper);

}

void loop() {
  Serial.print("Opening next file...");
  File pattern = SD.open(files[curr_file++]);
  if (!pattern) {
    Serial.print("failed: ");
    Serial.println(pattern.name());
    return;
  }
  Serial.print("ok: ");
  Serial.println(pattern.name());

  theta_stepper.setCurrentPosition(0);

  while (pattern.available() > 0) {
    while (pattern.peek() == '#') {
      pattern.readStringUntil('\n');
    }

    long positions[2];

    float theta, rho;

    theta = pattern.parseFloat(SKIP_ALL);
    rho = pattern.parseFloat(SKIP_ALL);

    Serial.println("moving");
    Serial.println(theta);
    Serial.println(rho);
    positions[0] = theta / 6.28 * STEPS_PER_REV;
    positions[1] = rho * STEPS_RHO;
    steppers.moveTo(positions);
    steppers.runSpeedToPosition(); // Blocks until all are in position
  }
  pattern.close();
  delay(1000);
}
