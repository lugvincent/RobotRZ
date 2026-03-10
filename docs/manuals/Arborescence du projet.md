Arborescence du projet
vue depuis Visual studio code - V PC
C:.
|   .env
|   .gitattributes
|   .gitignore
|   platformio.ini
|   RobotRZ.zip
|   termux.zip
|
+---.pio
|   +---build
|   |   |   project.checksum
|   |   |
|   |   \---megaatmega2560
|   |           idedata.json
|   |
|   \---libdeps
|       +---megaatmega2560
|       |   |   integrity.dat
|       |   |
|       |   +---Adafruit NeoPixel
|       |   |   |   .gitignore
|       |   |   |   .piopm
|       |   |   |   Adafruit_NeoPixel.cpp        
|       |   |   |   Adafruit_NeoPixel.h
|       |   |   |   Adafruit_Neopixel_RP2.cpp    
|       |   |   |   CONTRIBUTING.md
|       |   |   |   COPYING
|       |   |   |   esp.c
|       |   |   |   esp8266.c
|       |   |   |   kendyte_k210.c
|       |   |   |   keywords.txt
|       |   |   |   library.properties
|       |   |   |   psoc6.c
|       |   |   |   README.md
|       |   |   |   rp2040_pio.h
|       |   |   |
|       |   |   +---.github
|       |   |   |   |   ISSUE_TEMPLATE.md        
|       |   |   |   |   PULL_REQUEST_TEMPLATE.md 
|       |   |   |   |
|       |   |   |   \---workflows
|       |   |   |           githubci.yml
|       |   |   |
|       |   |   \---examples
|       |   |       +---buttoncycler
|       |   |       |       .esp8266.test.skip   
|       |   |       |       buttoncycler.ino     
|       |   |       |
|       |   |       +---RGBWstrandtest
|       |   |       |       .esp8266.test.skip   
|       |   |       |       .trinket.test.skip   
|       |   |       |       RGBWstrandtest.ino   
|       |   |       |
|       |   |       +---simple
|       |   |       |       .esp8266.test.skip   
|       |   |       |       simple.ino
|       |   |       |
|       |   |       +---simple_new_operator      
|       |   |       |       .esp8266.test.skip   
|       |   |       |       simple_new_operator.ino
|       |   |       |
|       |   |       +---strandtest
|       |   |       |       .esp8266.test.skip   
|       |   |       |       strandtest.ino       
|       |   |       |       
|       |   |       +---StrandtestArduinoBLE     
|       |   |       |       .none.test.only      
|       |   |       |       StrandtestArduinoBLE.ino
|       |   |       |
|       |   |       +---StrandtestArduinoBLECallback
|       |   |       |       .none.test.only      
|       |   |       |       StrandtestArduinoBLECallback.ino
|       |   |       |
|       |   |       +---StrandtestBLE
|       |   |       |       .none.test.only      
|       |   |       |       BLESerial.cpp        
|       |   |       |       BLESerial.h
|       |   |       |       StrandtestBLE.ino    
|       |   |       |
|       |   |       +---StrandtestBLE_nodelay    
|       |   |       |       .none.test.only      
|       |   |       |       BLESerial.cpp        
|       |   |       |       BLESerial.h
|       |   |       |       StrandtestBLE_nodelay.ino
|       |   |       |
|       |   |       +---strandtest_nodelay       
|       |   |       |       .esp8266.test.skip   
|       |   |       |       strandtest_nodelay.ino
|       |   |       |
|       |   |       \---strandtest_wheel
|       |   |               .esp8266.test.skip   
|       |   |               strandtest_wheel.ino 
|       |   |
|       |   +---Arduino_LSM6DS3
|       |   |   |   .codespellrc
|       |   |   |   .piopm
|       |   |   |   CHANGELOG
|       |   |   |   keywords.txt
|       |   |   |   library.properties
|       |   |   |   LICENSE.txt
|       |   |   |   README.adoc
|       |   |   |   
|       |   |   +---.github
|       |   |   |   |   dependabot.yml
|       |   |   |   |
|       |   |   |   \---workflows
|       |   |   |           check-arduino.yml
|       |   |   |           compile-examples.yml
|       |   |   |           report-size-deltas.yml
|       |   |   |           spell-check.yml      
|       |   |   |           sync-labels.yml      
|       |   |   |
|       |   |   +---examples
|       |   |   |   +---SimpleAccelerometer      
|       |   |   |   |       SimpleAccelerometer.ino
|       |   |   |   |       
|       |   |   |   +---SimpleGyroscope
|       |   |   |   |       SimpleGyroscope.ino  
|       |   |   |   |
|       |   |   |   \---SimpleTempSensor
|       |   |   |           SimpleTempSensor.ino 
|       |   |   |
|       |   |   \---src
|       |   |           Arduino_LSM6DS3.h        
|       |   |           LSM6DS3.cpp
|       |   |           LSM6DS3.h
|       |   |
|       |   +---Encoder
|       |   |   |   .piopm
|       |   |   |   Encoder.cpp
|       |   |   |   Encoder.h
|       |   |   |   keywords.txt
|       |   |   |   library.properties
|       |   |   |   README.md
|       |   |   |
|       |   |   +---examples
|       |   |   |   +---Basic
|       |   |   |   |       Basic.ino
|       |   |   |   |
|       |   |   |   +---NoInterrupts
|       |   |   |   |       NoInterrupts.ino     
|       |   |   |   |
|       |   |   |   +---SpeedTest
|       |   |   |   |       SpeedTest.ino        
|       |   |   |   |
|       |   |   |   \---TwoKnobs
|       |   |   |           TwoKnobs.ino
|       |   |   |
|       |   |   \---utility
|       |   |           direct_pin_read.h        
|       |   |           interrupt_config.h       
|       |   |           interrupt_pins.h
|       |   |
|       |   +---HX711
|       |   |   |   .gitignore
|       |   |   |   .piopm
|       |   |   |   .travis.yml
|       |   |   |   CONTRIBUTORS.md
|       |   |   |   keywords.txt
|       |   |   |   library.json
|       |   |   |   library.properties
|       |   |   |   LICENSE
|       |   |   |   Makefile
|       |   |   |   platformio.ini
|       |   |   |   README.md
|       |   |   |
|       |   |   +---doc
|       |   |   |       faq.md
|       |   |   |       notes.md
|       |   |   |       platformio-howto.md      
|       |   |   |
|       |   |   +---examples
|       |   |   |   +---HX711_basic_example      
|       |   |   |   |       HX711_basic_example.ino
|       |   |   |   |
|       |   |   |   +---HX711_full_example       
|       |   |   |   |       HX711_full_example.ino
|       |   |   |   |
|       |   |   |   +---HX711_retry_example      
|       |   |   |   |       HX711_retry_example.ino
|       |   |   |   |
|       |   |   |   \---HX711_timeout_example
|       |   |   |           HX711_timeout_example.ino
|       |   |   |
|       |   |   \---src
|       |   |           HX711.cpp
|       |   |           HX711.h
|       |   |
|       |   \---NewPing
|       |       |   .piopm
|       |       |   keywords.txt
|       |       |   library.properties
|       |       |   README.md
|       |       |
|       |       +---examples
|       |       |   +---NewPing15SensorsTimer    
|       |       |   |       NewPing15SensorsTimer.ino
|       |       |   |
|       |       |   +---NewPing3Sensors
|       |       |   |       NewPing3Sensors.ino  
|       |       |   |
|       |       |   +---NewPingEventTimer        
|       |       |   |       NewPingEventTimer.ino|       |       |   |
|       |       |   +---NewPingExample
|       |       |   |       NewPingExample.ino
|       |       |   |
|       |       |   +---NewPingTimerMedian       
|       |       |   |       NewPingTimerMedian.ino
|       |       |   |
|       |       |   \---TimerExample
|       |       |           TimerExample.ino
|       |       |
|       |       \---src
|       |               NewPing.cpp
|       |               NewPing.h
|       |
|       \---uno
|           |   integrity.dat
|           |
|           \---DualVNH5019MotorShield
|               |   .piopm
|               |   .travis.yml
|               |   DualVNH5019MotorShield.cpp   
|               |   DualVNH5019MotorShield.h     
|               |   keywords.txt
|               |   library.properties
|               |   LICENSE.txt
|               |   README.md
|               |
|               \---examples
|                   \---Demo
|                           Demo.ino
|
+---.vscode
|       c_cpp_properties.json
|       extensions.json
|       launch.json
|
+---docs
|   +---manuals
|   |   |   doc_A1_architecture_acteurs.md       
|   |   |   doc_A1_architecture_acteurs.odt      
|   |   |   doc_A2_flux_sequences.md
|   |   |   doc_A2_flux_sequences.odt
|   |   |   doc_B1_utilisateur_STT.md
|   |   |   doc_B1_utilisateur_STT.odt
|   |   |   doc_B2_dev_STT.md
|   |   |   doc_B2_dev_STT.odt
|   |   |   doc_C_reference_SE.md
|   |   |   doc_C_reference_SE.odt
|   |   |   table_A_SE.xlsx
|   |   |   table_A_structure_regles.docx        
|   |   |   table_A_structure_regles.md
|   |   |
|   |   \---provisoire
|   \---schemas
+---lib
|   |   RZlibrairiesPersoNew.zip
|   |
|   \---RZlibrairiesPersoNew
|       |   library.properties
|       |   RZlibrairiesPersoNew.h
|       |
|       \---src
|           |   RZlibrairiesPersoNew.cpp
|           |   RZlibrairiesPersoNew.h
|           |
|           +---actuators
|           |       lring.cpp
|           |       lring.h
|           |       lrub.cpp
|           |       lrub.h
|           |       mtr.cpp
|           |       mtr.h
|           |       srv.cpp
|           |       srv.h
|           |
|           +---communication
|           |       com.h
|           |       communication.cpp
|           |       communication.h
|           |       dispatch_CfgS.cpp
|           |       dispatch_com.cpp
|           |       dispatch_Ctrl_L.cpp
|           |       dispatch_FS.cpp
|           |       dispatch_IRmvt.cpp
|           |       dispatch_Lring.cpp
|           |       dispatch_Lrub.cpp
|           |       dispatch_Mic.cpp
|           |       dispatch_Mtr.cpp
|           |       dispatch_Odom.cpp
|           |       dispatch_SecMvt.cpp
|           |       dispatch_Srv.cpp
|           |       dispatch_Urg.cpp
|           |       dispatch_Us.cpp
|           |       old_dispatch_Lring.cpp       
|           |       vpiv_dispatch.cpp
|           |       vpiv_dispatch.h
|           |       vpiv_utils.cpp
|           |       vpiv_utils.h
|           |
|           +---config
|           |       config.cpp
|           |       config.h
|           |
|           +---control
|           |       ctrl_L.cpp
|           |       ctrl_L.h
|           |
|           +---hardware
|           |       fs_hardware.cpp
|           |       fs_hardware.h
|           |       lring_hardware.cpp
|           |       lring_hardware.h
|           |       lrub_hardware.cpp
|           |       lrub_hardware.h
|           |       mic_hardware.cpp
|           |       mic_hardware.h
|           |       mtr_hardware.cpp
|           |       mtr_hardware.h
|           |       mvt_ir_hardware.cpp
|           |       mvt_ir_hardware.h
|           |       odom_hardware.cpp
|           |       odom_hardware.h
|           |       srv_hardware.cpp
|           |       srv_hardware.h
|           |       us_hardware.cpp
|           |       us_hardware.h
|           |
|           +---navigation
|           |       odom.cpp
|           |       odom.h
|           |
|           +---safety
|           |       mvt_safe.cpp
|           |       mvt_safe.h
|           |
|           +---sensors
|           |       fs.cpp
|           |       fs.h
|           |       mic.cpp
|           |       mic.h
|           |       mvt_ir.cpp
|           |       mvt_ir.h
|           |       us.cpp
|           |       us.h
|           |
|           \---system
|                   urg.cpp
|                   urg.h
|
+---mqtt
|   +---certificates
|   \---topics
+---node-red
|   +---config
|   +---flows
|   |       flows.json
|   |
|   \---nodes
+---src
|   +---mega
|   |       main - Mega.cpp
|   |       main.cpp
|   |
|   \---uno
|           main - uno.cpp
|           main.cpp
|
+---tasker
|   |   rz_facial_scene.sh
|   |   rz_launch_app.sh
|   |
|   +---Profiles
|   |   |   rz_apps_manager.xml
|   |   |   rz_tts_task.xml
|   |   |   rz_zoom_profile.xml
|   |   |
|   |   \---rz_facial_expressions
|   |           angry.xml
|   |           neutral.xml
|   |           smile.xml
|   |
|   \---scripts
+---termux
|   +---config
|   |   |   check_config.sh
|   |   |   courant_init.json
|   |   |   courant_init.sh
|   |   |   global.json
|   |   |   keys.json
|   |   |   old_check_config.sh
|   |   |   original_init.sh
|   |   |
|   |   \---sensors
|   |           sys_config.json
|   |
|   +---docs
|   |       api_vpiv.md
|   |       architecture.md
|   |       installation.md
|   |
|   +---fifo
|   |       fifo_manager.sh
|   |
|   +---install
|   |       README.md
|   |       setup_python.sh
|   |       setup_termux.sh
|   |
|   +---log
|   +---scripts
|   |   +---appli
|   |   |       rz_appli_manager.sh
|   |   |
|   |   +---core
|   |   |       mqtt_bridge.py
|   |   |       rz_state-manager.sh
|   |   |       rz_vpiv_parser.sh
|   |   |       safety_stop.sh
|   |   |
|   |   +---sensors
|   |   |   +---cam
|   |   |   |       cam_config.json
|   |   |   |       rz_camera_manager.sh
|   |   |   |
|   |   |   +---gyro
|   |   |   |       gyro_config.json
|   |   |   |       rz_gyro_manager.sh
|   |   |   |
|   |   |   +---Mg
|   |   |   |       rz_mg_manager.sh
|   |   |   |
|   |   |   +---mic
|   |   |   |       mic_config.json
|   |   |   |       rz_balayage_sonore.sh        
|   |   |   |       rz_microSe_manager.sh        
|   |   |   |
|   |   |   +---stt
|   |   |   |       old-rz_stt_manager.py        
|   |   |   |       rz_ai_conversation.py        
|   |   |   |       rz_build_dict.py
|   |   |   |       rz_build_system.py
|   |   |   |       rz_stt_handler.sh
|   |   |   |       rz_stt_manager.py
|   |   |   |       rz_stt_manager.sh
|   |   |   |       stt_catalog.json
|   |   |   |
|   |   |   +---sys
|   |   |   |   |   rz_sys_monitor.sh
|   |   |   |   |
|   |   |   |   \---lib
|   |   |   |           alerts_sys.sh
|   |   |   |           urgence.sh
|   |   |   |
|   |   |   \---torch
|   |   |           rz_torch_manager.sh
|   |   |
|   |   +---tests
|   |   |       old_test_mqtt.py
|   |   |       old_test_stt.sh
|   |   |       test_gyro_simulator.sh
|   |   |       test_mg_simulator.sh
|   |   |
|   |   \---utils
|   |           error_handler.sh
|   |           fifo.manager.sh
|   |           logger.sh
|   |           rz_tasker_bridge.sh
|   |
|   +---services
|   \---taskerScripts
\---tools
    +---backup
    \---deploy