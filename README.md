# QtAda

## Introduction

QtAda is a tool designed for testing Graphical User Interface (GUI) applications developed with the Qt framework. This application supports testing environments either created through QtWidgets or QtQuick/QML interfaces. QtAda is specifically tested and optimized for Linux, with a primary focus on distributions like Arch and Manjaro Linux.

## Table of Contents

- [QtAda](#qtada)
  - [Introduction](#introduction)
  - [Table of Contents](#table-of-contents)
  - [Features](#features)
  - [Installation](#installation)
  - [Usage](#usage)
    - [Console Usage](#console-usage)
    - [GUI Usage](#gui-usage)
  - [Command Documentation](#command-documentation)
    - [General Commands](#general-commands)
      - [Verification](#verification)
      - [Sleep Commands](#sleep-commands)
    - [Mouse and Keyboard Events](#mouse-and-keyboard-events)
      - [Mouse Events](#mouse-events)
      - [Keyboard and Wheel Events (unstable)](#keyboard-and-wheel-events-unstable)
    - [Button Interactions](#button-interactions)
    - [Advanced Click Commands](#advanced-click-commands)
    - [Selection Commands](#selection-commands)
    - [View Commands](#view-commands)
    - [Value Manipulation](#value-manipulation)
    - [Text Manipulation](#text-manipulation)
    - [Close Commands](#close-commands)
  - [Configuration](#configuration)
  - [Troubleshooting](#troubleshooting)
  - [Contributors](#contributors)
  - [License](#license)

## Features

QtAda's core functionality revolves around automating the recording and saving of test scripts written in JavaScript, based on user interactions within a GUI. It also facilitates the playback of these scripts to evaluate the test results. Users can perform checks on object meta-properties during the recording phase. By selecting a graphical object or an object from the element tree, users can confirm checks of desired meta-properties, which are then included in the generated test script as `QtAda.verify`.

- **Automatic Script Recording:** Capture user interactions within the GUI to generate test scripts.
- **Script Editing:** Modify and enhance generated scripts manually. Care must be taken when editing automatically generated commands.
- **Verification Checks:** During script recording, add checks for object meta-properties to ensure GUI elements adhere to expected standards.

**Note:** QtAda is currently in early development stages and may be prone to various errors. Please proceed with caution.

## Installation

_Coming soon._

## Usage

### Console Usage

The console interface is ideal for integrating QtAda's automated test runs into your project's automated tests:
- Preferred for embedding in continuous testing environments.
- Supports various command-line arguments which can be explored using `QtAda --help`.

### GUI Usage

The GUI provides a more visual and interactive approach, preferred for creating and managing test scripts:
- **Project File Management:** Create `.qtada` project files to manage test scripts and other necessary files.
- **Script Recording Settings:** Each script recorded via the GUI can have its own unique recording and playback settings.
- **Integrated Text Editor:** Edit scripts directly within the built-in text editor.
- **File Management:** Basic file operations like renaming, deleting, and displaying files in the directory are supported within the GUI.

## Command Documentation

This section outlines the commands available in QtAda for reproducing test actions within a Qt application interface. Note that all commands need to be prefixed with `QtAda.` when used in scripts. The first argument in most commands (except for the sleep functions) is the path to the GUI object, which helps identify the target object in the GUI. Additionally, unless otherwise specified, all functions apply to the classes mentioned and their descendants.

### General Commands

This section details the available commands in QtAda for simulating user actions within a Qt application interface. Note that all commands must be prefixed with `QtAda.` when used in scripts, although they are listed here without this prefix for simplicity. All functions are applicable to the described classes and their descendants, and the first argument, `path` (string), identifies the target GUI object.

#### Verification
- `verify(path, property, value)`
  - **Purpose:** Verifies if the specified meta-property of an object matches a given value.
  - **Arguments:**
    - `property` (string): The property name to verify.
    - `value` (string): The expected value of the property.

#### Sleep Commands 
- `sleep(sec)`
  - **Purpose:** Pauses the script execution for a specified number of seconds without pausing the application.
  - **Arguments:**
    - `sec` (integer): Time in seconds.

- `msleep(msec)`
  - **Purpose:** Pauses the script execution for a specified number of milliseconds.
  - **Arguments:**
    - `msec` (integer): Time in milliseconds.

- `usleep(usec)`
  - **Purpose:** Pauses the script execution for a specified number of microseconds.
  - **Arguments:**
    - `usec` (integer): Time in microseconds.

_Note: These are not auto-generated and should be used as needed._

### Mouse and Keyboard Events

#### Mouse Events
- `mouseClick(path, mouseButton, x, y)`
  - **Purpose:** Simulates a mouse click on any GUI component.
  - **Arguments:**
    - `mouseButton` (string): Mouse button used (`LeftButton`, `RightButton`, `MiddleButton`, `BackButton`, `ForwardButton`, `NoButton`).
    - `x`, `y` (integer): Coordinates relative to the object.

- `mouseDblClick(path, mouseButton, x, y)`
  - **Purpose:** Simulates a double mouse click on any GUI component.

#### Keyboard and Wheel Events (unstable)
- `keyEvent(path, keyText)`
  - **Purpose:** Simulates a keyboard event (unstable in this version, use with caution).
  - **Arguments:**
    - `keyText` (string): The key or combination of keys pressed.

- `wheelEvent(path, dx, dy)`
  - **Purpose:** Simulates a mouse wheel scroll (unstable in this version, use with caution).
  - **Arguments:**
    - `dx`, `dy` (integer): Horizontal and vertival scroll amount in pixels.

### Button Interactions
- `buttonClick(path)`
  - **Purpose:** Simulates a click on any button-like component derived from `QAbstractButton` and `QML Button`.

- `buttonDblClick(path)`
  - **Purpose:** Simulates a double click on any button-like component.

- `buttonPress(path)`
  - **Purpose:** Simulates a press event on any button-like component.

- `mouseAreaClick(path)`
  - **Purpose:** Simulates a click on a `QML MouseArea`.

- `mouseAreaDblClick(path)`
  - **Purpose:** Simulates a double click on a `QML MouseArea`.

- `mouseAreaPress(path)`
  - **Purpose:** Simulates a press event on a `QML MouseArea`.

- `checkButton(path, isChecked)`
  - **Purpose:** Toggles any button-like component's checked state to `isChecked` if it is `checkable`.
  - **Arguments:**
    - `isChecked` (boolean): Desired state of the button (`true` or `false`).

- `setDelayProgress(path, progress)`
  - **Purpose:** Sets progress to a `QML DelayButton`.
  - **Arguments:**
    - `progress` (real): Progress value (0.0 - 1.0).

### Advanced Click Commands
- `triggerAction(path)`
  - **Purpose:** Triggers an `QAction` at the specified path.

- `triggerAction(path, isChecked)`
  - **Purpose:** Toggles any `QAction` checked state to `isChecked` if it is `checkable`.
  - **Arguments:**
    - `isChecked` (boolean): Desired state of the button (`true` or `false`).

- `delegateClick(path, index)`
  - **Purpose:** Simulates a click on a delegate of any view-like component derived from a `QML`.
  - **Arguments:**
    - `index` (integer): Index of the delegate within the component.

- `delegateDblClick(path, index)`
  - **Purpose:** Simulates a double click on a delegate of any view-like component derived from a `QML`.

- `delegateClick(path, indexPath)`
  - **Purpose:** Simulates a click on a delegate in a `QTreeView`.
  - **Arguments:**
    - `indexPath` (list of integers): Array of indices representing the path to the delegate.

- `delegateDblClick(path, indexPath)`
  - **Purpose:** Simulates a double click on a delegate in a `QTreeView`.

- `delegateClick(path, row, column)`
  - **Purpose:** Simulates a click on a delegate in an `QAbstractItemView` (excluding `QTreeView`).
  - **Arguments:**
    - `row` (integer): Row index of the delegate in the model.
    - `column` (integer): Column index of the delegate in the model.

- `delegateDblClick(path, row, column)`
  - **Purpose:** Simulates a double click on a delegate in a `QAbstractItemView` (excluding `QTreeView`).

### Selection Commands
- `selectItem(path, index)`
  - **Purpose:** Selects an item in components like `QComboBox`, `QML ComboBox` or `QML Tumbler` based on index.
  - **Arguments:**
    - `index` (integer): Index of the item to select.

- `selectItem(path, text)`
  - **Purpose:** Selects an item by its text description in components like `QComboBox` or `QML ComboBox`.
  - **Arguments:**
    - `text` (string): Text description of the item to select. If multiple items share the same description, the first encountered item is selected.

- `selectItem(path, text, index)`
  - **Purpose:** Attempts to select an item by its text description in components like `QComboBox` or `QML ComboBox`. If no item matches the text, it selects the item at the specified index as a fallback.
  - **Arguments:**
    - `text` (string): Text description of the item. This method will attempt to find the first item that matches this description.
    - `index` (integer): Index to be used as a fallback if no items match the text description.

- `selectTabItem(path, index)`
  - **Purpose:** Selects a tab based on its index in components like `QTabBar`.
  - **Arguments:**
    - `index` (integer): Index of the tab to select.

- `selectTabItem(path, text)`
  - **Purpose:** Selects a tab by its text description in components like `QTabBar`.
  - **Arguments:**
    - `text` (string): Text description of the tab to select. If multiple tabs have the same description, the first one encountered is selected.

- `selectTabItem(path, text, index)`
  - **Purpose:** Attempts to select a tab by its text description in components like `QTabBar`. If no tab matches the text, it selects the tab at the specified index as a fallback.
  - **Arguments:**
    - `text` (string): Text description of the tab. This method will attempt to find the first tab that matches this description.
    - `index` (integer): Index to be used as a fallback if no tabs match the text description.

### View Commands
- `setSelection(path, selectionData)`
  - **Purpose:** Selects model elements in a `QAbstractItemView` based on a structured array of selection data.
  - **Arguments:**
    - `selectionData` (array): An array of objects specifying selection criteria. Each object should define `row` as `"ALL"` (for all rows) or `integer` (row index), and `column` as `"ALL"` (for all columns), an `integer` (column index), or an `array of integers` (multiple column indices).

- `clearSelection(path)`
  - **Purpose:** Clears the selection in a `QAbstractItemView` without removing any items from the model.

- `expandDelegate(path, indexPath)`
  - **Purpose:** Expands a delegate in a `QTreeView`.
  - **Arguments:**
    - `indexPath` (list of integers): Array of indices representing the path to the delegate.

- `collapseDelegate(path, indexPath)`
  - **Purpose:** Collapses a delegate in a `QTreeView`.

- `undoCommand(path, index)`
  - **Purpose:** Issues an undo command in a `QUndoView`. _Currently unstable in QtAda._
  - **Arguments:**
    - `index` (integer): Index of the command to undo.

- `selectViewItem(path, index)`
  - **Purpose:** Sets a currnet item in a `QML PathView` or `QML SwipeView` based on its index.
  - **Arguments:**
    - `index` (integer): Index of the item to set.

### Value Manipulation
- `setValue(path, value)`
  - **Purpose:** Sets the current numeric value in components like `QSpinBox`, `QDoubleSpinBox`, `QML SpinBox`, `QAbstractSlider`, `QML Slider`, and `QML ScrollBar`. If the component only accepts integer values, the fractional part of the number will be discarded.
  - **Arguments:**
    - `value` (double): The numeric value to set.

- `setValue(path, leftValue, rightValue)`
  - **Purpose:** Sets both the left and right values of a `QML RangeSlider`.
  - **Arguments:**
    - `leftValue` (double): The left boundary value to set.
    - `rightValue` (double): The right boundary value to set.

- `setValue(path, dateTime)`
  - **Purpose:** Sets the date and/or time in components derived from `QDateTime` or `QCalendarWidget`. The date/time string must be in ISO format. Unnecessary parts of the string will be ignored if the component does not require them.
  - **Arguments:**
    - `dateTime` (string): The ISO format date/time string to set.

- `setValue(path, value)`
  - **Purpose:** Sets a text-based value in `QML SpinBox` components that have overridden the `textFromValue` and `valueFromText` functions. For more details on these functions, see [Qt documentation on QML SpinBox](https://doc.qt.io/qt-5/qml-qtquick-controls2-spinbox.html#textFromValue-prop).
  - **Arguments:**
    - `value` (string): The text representation of the value to set.

- `changeValue(path, type)`
  - **Purpose:** Modifies the value of components like `QSpinBox`, `QDoubleSpinBox`, `QAbstractSlider`, and `QML SpinBox` according to the specified type.
  - **Arguments:**
    - `type` (string): Type of modification, with options including:
      - `Up`: Increases the value by a small, predefined step.
      - `DblUp`: Increases the value by a small, predefined step two times.
      - `Down`: Decreases the value by a small, predefined step.
      - `DblDown`: Decreases the value by a small, predefined step two times.
      - `SingleStepAdd`: Increases the value by a small, predefined step (specific to `QAbstractSlider`).
      - `SingleStepSub`: Decreases the value by a small, predefined step (specific to `QAbstractSlider`).
      - `PageStepAdd`: Increases the value by a larger, page-size step (specific to `QAbstractSlider`).
      - `PageStepSub`: Decreases the value by a larger, page-size step (specific to `QAbstractSlider`).
      - `ToMinimum`: Sets the value to the minimum limit of the range (specific to `QAbstractSlider`).
      - `ToMaximum`: Sets the value to the maximum limit of the range (specific to `QAbstractSlider`).

### Text Manipulation
- `setText(path, text)`
  - **Purpose:** Sets text in components derived from `QTextEdit`, `QLineEdit`, `QPlainTextEdit`, `QKeySequenceEdit`, `QML TextEdit`, or `QML TextInput`.
  - **Arguments:**
    - `text` (string): The text to be set in the component.

- `setText(path, indexPath, text)`
  - **Purpose:** Sets text in a specific delegate within a `QTreeView` component.
  - **Arguments:**
    - `indexPath` (list of integers): Array of indices representing the path to the delegate.

- `setText(path, row, column, text)`
  - **Purpose:** Sets text in an element of a model within a `QAbstractItemView` (excluding `QTreeView`).
  - **Arguments:**
    - `row` (integer): Row index of the delegate in the model.
    - `column` (integer): Column index of the delegate in the model.

### Close Commands
- `closeDialog(path)`
  - **Purpose:** Closes a dialog based on `QDialog`.

- `closeWindow(path)`
  - **Purpose:** Closes a window based on `QMainWindow` or `QML Window`.

_Remember to add the `QtAda.` prefix when using these commands in your scripts._

## Configuration

_Coming soon._

## Troubleshooting

_Coming soon._

## Contributors

_Coming soon._

## License

_Coming soon._
