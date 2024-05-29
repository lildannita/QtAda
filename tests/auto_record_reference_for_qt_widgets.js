function test() {
    // Looks like QComboBox container clicked
    // QtAda.mouseClick('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=comboBox_0', 'LeftButton', 355, 18);
    QtAda.selectItem('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=comboBox_0', 2);
    QtAda.setText('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=editableComboBox_0', 'New Text For ComboBox');
    QtAda.mouseClick('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=label_0', 'LeftButton', 355, 8);
    // Looks like QComboBox container clicked
    // QtAda.mouseClick('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=fontComboBox_0', 'LeftButton', 701, 19);
    QtAda.selectItem('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=fontComboBox_0', 2);
    QtAda.setText('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=spinBox_0', '22');
    QtAda.mouseClick('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=label_0', 'LeftButton', 355, 8);
    QtAda.changeValue('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=spinBox_0', 'Up');
    QtAda.setText('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=doubleSpinBox_0', '3,22');
    QtAda.mouseClick('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=label_0', 'LeftButton', 355, 8);
    QtAda.changeValue('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=doubleSpinBox_0', 'Down');
    QtAda.buttonClick('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=disabledRadioButton_0'); // Button text: 'Disabled RadioButton'
    QtAda.buttonClick('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=firstEnabledRadio_0'); // Button text: 'First Enabled RadioButton'
    QtAda.buttonClick('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=secondEnabledRadio_0'); // Button text: 'Second Enabled RadioButton'
    QtAda.checkButton('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=firstCheckBox_0', true); // Button text: CheckBox1
    QtAda.checkButton('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=thirdCheckBox_0', true); // Button text: CheckBox3
    QtAda.checkButton('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=firstCheckBox_0', false); // Button text: CheckBox1
    QtAda.checkButton('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=checkablePushButton_0', true); // Button text: Checkable Button
    QtAda.buttonClick('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=simplePushButton_0'); // Button text: 'Simple Button'
    QtAda.checkButton('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=checkablePushButton_0', false); // Button text: Checkable Button
    QtAda.setValue('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=calendarWidget_0', '2022-02-23');
    QtAda.setValue('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=calendarWidget_0', '2022-01-31');
    QtAda.setText('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=dateTimeEdit_0', '2022-02-01T22:22:00');
    QtAda.mouseClick('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=label_0', 'LeftButton', 355, 8);
    QtAda.setText('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=timeEdit_0', '00:22:00');
    QtAda.mouseClick('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=label_0', 'LeftButton', 355, 8);
    QtAda.setText('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=dateEdit_0', '2022-02-01');
    QtAda.mouseClick('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=label_0', 'LeftButton', 355, 8);
    QtAda.setValue('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=dial_0', 35);
    QtAda.changeValue('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=horizontalScrollBar_0', 'PageStepAdd');
    QtAda.changeValue('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_0/n=verticalSlider_0', 'PageStepAdd');
    QtAda.buttonClick('n=MainWindow_0/n=toolBar_0/c=QToolButton_0'); // Button text: 'Open Simple Dialog'
    QtAda.buttonClick('n=MainWindow_0/c=Dialog_0/c=QPushButton_0'); // Button text: 'OK'
    QtAda.buttonClick('n=MainWindow_0/n=toolBar_0/c=QToolButton_1'); // Button text: 'Open Another Dialog'
    QtAda.buttonClick('n=MainWindow_0/c=Dialog_1/c=QPushButton_1'); // Button text: 'Open'
    QtAda.buttonClick('n=MainWindow_0/c=Dialog_1/c=Dialog_0/c=QPushButton_0'); // Button text: 'OK'
    QtAda.buttonClick('n=MainWindow_0/c=Dialog_1/c=QPushButton_0'); // Button text: 'OK'
    QtAda.buttonClick('n=MainWindow_0/n=toolBar_0/c=QToolButton_2'); // Button text: 'ToolButton2'
    QtAda.selectTabItem('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_tabbar_0', 1);
    QtAda.buttonClick('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_7_0/n=toolBox_0/c=QScrollArea_0/n=qt_scrollarea_viewport_0/n=page_5_0/n=toolBoxButton_0'); // Button text: 'Simple Button'
    QtAda.buttonClick('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_7_0/n=toolBox_0/n=qt_toolbox_toolboxbutton_1'); // Button text: 'Page With Radio Buttons'
    QtAda.buttonClick('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_7_0/n=toolBox_0/c=QScrollArea_1/n=qt_scrollarea_viewport_0/n=page_6_0/n=toolBoxFirstRadio_0'); // Button text: 'RadioButton'
    QtAda.buttonClick('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_7_0/n=toolBox_0/c=QScrollArea_1/n=qt_scrollarea_viewport_0/n=page_6_0/n=toolBoxSecondRadio_0'); // Button text: 'RadioButton'
    QtAda.selectTabItem('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_tabbar_0', 2);
    QtAda.setText('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_2_0/n=textEdit_0', 'SampleText');
    QtAda.setText('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_2_0/n=plainTextEdit_0', 'SampleText');
    QtAda.setText('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_2_0/n=scrollArea_0/n=qt_scrollarea_viewport_0/n=scrollAreaWidgetContents_0/n=lineEdit_0', 'SampleText');
    QtAda.selectTabItem('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_tabbar_0', 3);
    QtAda.delegateDblClick('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_3_0/n=tableView_0', 0, 0); // Delegate text: 'Ячейка 0, 0'
    QtAda.setText('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_3_0/n=tableView_0', 0, 0, 'Test Text');
    QtAda.delegateClick('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_3_0/n=tableView_0', 1, 1); // Delegate text: 'Ячейка 1, 1'
    QtAda.setSelection('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_3_0/n=tableView_0/c=QHeaderView_1', [{row: 'ALL', column: 2}]);
    QtAda.setSelection('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_3_0/n=tableWidget_0/c=QHeaderView_0', [{row: 3, column: 'ALL'}]);
    QtAda.selectTabItem('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_tabbar_0', 4);
    QtAda.expandDelegate('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_4_0/n=treeView_0', [0]); // Delegate text: 'Элемент 0'
    QtAda.delegateDblClick('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_4_0/n=treeView_0', [0, 0]); // Delegate text: 'Подэлемент 0'
    QtAda.setText('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_4_0/n=treeView_0', [0, 0], 'Test Text');
    QtAda.delegateClick('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_4_0/n=treeView_0', [1]); // Delegate text: 'Элемент 1'
    QtAda.expandDelegate('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_4_0/n=treeWidget_0', [0]); // Delegate text: 'Элемент 0'
    QtAda.collapseDelegate('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_4_0/n=treeWidget_0', [0]); // Delegate text: 'Элемент 0'
    QtAda.expandDelegate('n=MainWindow_0/n=centralwidget_0/n=tabWidget_0/n=qt_tabwidget_stackedwidget_0/n=tab_4_0/n=treeView_0', [0, 0]); // Delegate text: 'Test Text'
    // Looks like useless menu click
    // QtAda.triggerAction('n=MainWindow_0/n=menuBar_0/n=menuMenu_0/c=QAction_0'); // Action text: 'Menu'
    QtAda.triggerAction('n=MainWindow_0/n=actionTest_switch_for_tab_3_0', true); // Action text: 'Switch Action'
    // Looks like useless menu click
    // QtAda.triggerAction('n=MainWindow_0/n=menuBar_0/n=menuSettings_0/c=QAction_0'); // Action text: 'Settings'
    QtAda.triggerAction('n=MainWindow_0/n=actiontest_0'); // Action text: 'Another Simple Action'
}
test();
