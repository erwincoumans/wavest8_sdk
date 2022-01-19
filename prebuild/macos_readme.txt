If the app doesn't open on MacOS (it maybe reported as 'malicious' or 'corrupted') 
open a terminal window and use
xattr -dr com.apple.quarantine wavest8_editor*
or 
xattr -dr wavest8_editor.app (replace by actual app name)

