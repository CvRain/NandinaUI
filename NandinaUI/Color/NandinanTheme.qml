pragma Singleton

import QtQuick 2.15

QtObject {
    property string latte: "theme::catppuccin_latte"
    property string frappe: "theme::catppuccin_frappe"
    property string macchiato: "theme::catppuccin_macchiato"
    property string mocha: "theme::catppuccin_mocha"

    function printTheme() {
        console.debug(latte)
        console.debug(frappe)
        console.debug(macchiato)
        console.debug(mocha)
    }
}
