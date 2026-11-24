# Warehouse (vault)

- **Maximum Zen in the warehouse: 1,000,000,000,000 (1 trillion).** The server enforces it (`MaximumVaultMoney` in the game configuration;
  the vault Zen is a 64-bit number and travels as 8 bytes in the `0xC1 0x81` packet). The inventory stays capped at 2,147,483,647 Zen,
  so one deposit or withdrawal moves at most that much. The warehouse window shows the maximum as a tooltip when the mouse is over
  the Zen box (`CNewUIStorageInventory::MAX_STORAGE_ZEN`; keep it equal to the server value).
- **Ten vaults.** When the account has more than one vault, two arrow buttons beside the round Zen / lock buttons switch to the previous or
  next vault. They use the same bevelled button frame as the round buttons (art: `newui_vault_btn_L/R`, drawn by
  `tools/ui_hd/vault_buttons.py` from the frame of `newui_Bt_lock`, 24 x 29 pixels, with the 2x and 4x tiers). The older `newui_vault_arrow_*`
  textures are still used by the Jewel Bank page arrows.
- **Extended vault.** With an extended vault the second warehouse opens with `H` or the expand button.
