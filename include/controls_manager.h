#ifndef CONTROLS_MANAGER_H
#define CONTROLS_MANAGER_H

#include <Arduino.h>

namespace ControlsManager {
    enum ControlState {
        STATE_NAV,  // Moving the menu cursor
        STATE_EDIT  // Editing a selected parameter value
    };

    // Initialize pins and interrupts for the rotary encoder and switch
    void init();

    // Read input states and update menu selection/parameter values
    void update();

    // Accessors for UI rendering
    ControlState getState();
    int getMenuCursor();
    bool isEditing();
}

#endif // CONTROLS_MANAGER_H
