#pragma once

void web_ui_init();   // starts WiFi AP + HTTP server
void web_ui_poll();   // call from loop() — handles any deferred work
bool web_ui_has_override();  // true when web has sent params recently
