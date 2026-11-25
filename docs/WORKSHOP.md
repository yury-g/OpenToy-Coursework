# A first OpenToy workshop

Prepare and test each kit before class. A student should be able to open the Mac app, connect to their named toy, change behavior and see acknowledgments without using Arduino IDE or a terminal. Tools/commands in SETUP.md are for the instructor or the student's authorized AI agent.

## Lesson 1: change behavior without reflashing

Connect the toy over Bluetooth. On a bare board, change blink period and duty cycle, then request status. On the OLED board, choose eyes/closed/test/off, change brightness, then compare the acknowledged values. The board's USB readback is an independent check; a success message in the sending app is not enough.

Teach three separate questions: Did the client send the command? Did the toy accept and report it? Did the physical effect look/feel correct? Remote serial/I2C tests answer the first two. Someone must observe the display and touch response for the third.

## Lesson 2: expressive controls

With the physical controller compatibility checklist completed, choose the class toy and enable controller input; the app first confirms Touch off. Move the white shapes up/down with the left stick. Move pupils inside them with the D-pad. The right stick left/right winks that eye; up closes both from the bottom; down closes both from the top. Releasing opens the eyelids. Try combining all three controls, then A/B/X/Y expressions. Stop controller input before manual screen controls. Try unplugging the controller and reconnecting the toy.

The UI/controller adapter owns input mappings. Firmware owns bounded face parameters and rendering. Bluetooth owns delivery. Students can change the button mapping without changing radio/update code; they can change the eye drawing without rewriting controller recognition.

## Lesson 3: one small AI-assisted firmware edit

Ask the AI agent to change one expression's rendering while preserving command bounds, the Bluetooth services, hardware identity, partition layout, updater and deferred boot validation. Review the source change. Use **Build + update**. Compilation must succeed before transfer; the app validates the device-specific manifest, transfers the image, reboots, reconnects and checks the new build label.

If an update is unverified, check the running build and retry from byte zero. Keep a known-good source and USB cable available. Exercise rollback with a dedicated test image only during instructor preparation, not as the first student activity.

## Repeatability gate

This has passed bench tests on two boards on one Mac. Before classroom rollout, test fresh provisioning and installation on another Mac, then a second student kit. Record exact board/controller versions, app version, observed physical behavior and recovery results. An interrupted upload and failed-boot rollback should be recoverable without losing the prior working toy.

Motors, power distribution and physical motion are a separate stage. They must not inherit eye-control safety assumptions. Servo selection/power investigation is being developed separately.
