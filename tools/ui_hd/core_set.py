"""The phase-1 texture set (core in-game windows), as paths relative to Data/Interface, grouped by owner window."""

FRAME_KIT = [
    "newui_msgbox_back.OZJ",
    "newui_item_back01.OZT",
    "newui_item_back04.OZT",
    "newui_item_back02-L.OZT",
    "newui_item_back02-R.OZT",
    "newui_item_back03.OZT",
    "newui_item_table01(L).OZT",
    "newui_item_table01(R).OZT",
    "newui_item_table02(L).OZT",
    "newui_item_table02(R).OZT",
    "newui_item_table03(Up).OZT",
    "newui_item_table03(Dw).OZT",
    "newui_item_table03(L).OZT",
    "newui_item_table03(R).OZT",
    "newui_exit_00.OZT",
]

BOTTOM_BAR = [
    "newui_menu01.OZJ",
    "newui_menu02.OZJ",
    "partCharge1/newui_menu03.OZJ",
    "newui_menu02-03.OZJ",
    "newui_menu_blue.OZJ",
    "newui_menu_green.OZJ",
    "newui_menu_red.OZJ",
    "newui_menu_ag.OZJ",
    "newui_menu_sd.OZJ",
    "newui_exbar.OZJ",
    "Exbar_Master.OZJ",
    "partCharge1/newui_menu_Bt01.OZJ",
    "partCharge1/newui_menu_Bt02.OZJ",
    "partCharge1/newui_menu_Bt03.OZJ",
    "partCharge1/newui_menu_Bt04.OZJ",
    "partCharge1/newui_menu_Bt05.OZJ",
    "newui_skillbox.OZJ",
    "newui_skillbox2.OZJ",
    "newui_skill.OZJ",
    "newui_skill2.OZJ",
    "newui_skill3.OZJ",
    "newui_command.OZJ",
    "newui_non_skill.OZJ",
    "newui_non_skill2.OZJ",
    "newui_non_skill3.OZJ",
    "newui_non_command.OZJ",
]

CHARACTER_INFO = [
    "newui_cha_textbox02.OZT",
    "newui_chainfo_btn_level.OZT",
    "newui_chainfo_btn_quest.OZT",
    "newui_chainfo_btn_pet.OZT",
    "newui_chainfo_btn_master.OZT",
]

INVENTORY = [
    "newui_item_boots.OZT",
    "newui_item_cap.OZT",
    "newui_item_fairy.OZT",
    "newui_item_wing.OZT",
    "newui_item_weapon(L).OZT",
    "newui_item_weapon(R).OZT",
    "newui_item_upper.OZT",
    "newui_item_gloves.OZT",
    "newui_item_lower.OZT",
    "newui_item_ring.OZT",
    "newui_item_necklace.OZT",
    "newui_item_money.OZT",
    "newui_repair_00.OZT",
    "newui_expansion_btn.OZT",
    "newui_Bt_openshop.OZT",
    "newui_Bt_closeshop.OZT",
    "newui_item_box.OZT",
    "newui_inven_usebox_01.OZT",
    "newui_inven_usebox_02.OZT",
    "newui_inven_usebox_03.OZT",
]

LOGIN_SCENE = [
    "b_create.OZT",
    "b_connect.OZT",
    "b_delete.OZT",
    "server_menu_b_all.OZT",
    "server_credit_b_all.OZT",
    "cha_bt.OZT",
    "cha_id.OZT",
    "character_ex.OZT",
    "deco.OZT",
    "server_b2_all.OZT",
    "server_deco_all.OZT",
    "login_back.OZT",
    "login_me.OZT",
    "cr_mu_lo.OZT",
    "server_ex01.OZT",
]

CHAT_AND_GEMS = [
    "newui_Bt_Chat_guild.OZJ", "newui_Bt_Chat_normal.OZJ", "newui_Bt_Chat_party.OZJ", "newui_Bt_Chat_system.OZJ",
    "newui_chat_btn_alpha.OZJ", "newui_chat_btn_size.OZJ", "newui_chat_chat_on.OZJ", "newui_chat_frame_on.OZJ",
    "newui_chat_gens_on.OZJ", "newui_chat_guild_on.OZJ", "newui_chat_normal_on.OZJ", "newui_chat_party_on.OZJ",
    "newui_chat_system_on.OZJ", "newui_chat_whisper_on.OZJ", "newui_bt_Off_chat.OZJ",
    "newui_statusicon.OZJ",
]

GROUPS = {
    "frame_kit": FRAME_KIT,
    "bottom_bar": BOTTOM_BAR,
    "character_info": CHARACTER_INFO,
    "inventory": INVENTORY,
    "login_scene": LOGIN_SCENE,
    "chat": CHAT_AND_GEMS,
}

CORE_SET = [path for group in GROUPS.values() for path in group]
