SET(SOURCES ui_comp_label1.c
    ui_comp.c
    ui_Screen1.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    fonts/ui_font_MontserratMedium48.c
    fonts/ui_font_MontserratMedium60.c
    fonts/ui_font_MontserratSemiBold60.c
    fonts/ui_font_MontserratSemibold20.c
    fonts/ui_font_MontserratSemibold30.c)

add_library(ui ${SOURCES})
