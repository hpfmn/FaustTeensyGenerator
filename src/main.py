import json
import os
import os.path

fI = "fI"

variables = ''
patch = 'float *Patch[] = {\n'
updateFunc = "result updateFParam() {\n\tfor(int v = 0; v < VOICES; v++) {\n"
updateGyro = """void update_gyro() {
\tWire1.beginTransmission(MPU_ADDR);
\tWire1.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
\tWire1.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
\tWire1.requestFrom(MPU_ADDR, 7*2, true); // request a total of 7*2=14 registers
\t
\t// "Wire1.read()<<8 | Wire1.read();" means two registers are read and stored in the same variable
\taccelerometer_0 = Wire1.read()<<8 | Wire1.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
\taccelerometer_1 = Wire1.read()<<8 | Wire1.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
\taccelerometer_2 = Wire1.read()<<8 | Wire1.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
\ttemperature = Wire1.read()<<8 | Wire1.read(); // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
\tgyro_0 = Wire1.read()<<8 | Wire1.read(); // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
\tgyro_1 = Wire1.read()<<8 | Wire1.read(); // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
\tgyro_2 = Wire1.read()<<8 | Wire1.read(); // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)

\tfor(int v = 0; v < VOICES; v++) {
"""

updateAnalog = "void update_analog() {\n\tfor(int v = 0; v < VOICES; v++) {\n"
usesAnalog = False
usesGyro = False
maxdepth = 0
fixArduinoMenu = False;

def variable_name(item_label):
    label = item_label.replace(" ","_").replace("\t","_")
    return f"{label.lower()}"

def fix_arduino_menu_direction(menu):
    global fixArduinoMenu
    if not fixArduinoMenu:
        return menu
    # Stupid things because how arduinomenu works
    # Reverse Menu
    menu = menu.split("{")
    to_reverse = menu[1].split("\n")
    to_reverse.reverse()
    print(to_reverse)
    # Roll by 1
    to_reverse = [""] + [to_reverse[-2]] + to_reverse[1:-2] + [""]
    menu = menu[0] + "{" + "\n".join(to_reverse)

    return menu

def generateMenu(items, menulabel, top=True, cur_depth=1):
    global variables, patch, updateFunc, maxdepth, updateAnalog, usesAnalog, updateGyro, usesGyro
    submenus = []
    generated_submenus = []
    if cur_depth > maxdepth:
        maxdepth += 1

    menu = f'prompt* p_{variable_name(menulabel)}[] = {{\n'

    for item in items:
        print(item)
        if item['label'] == 'gate' or item['label'] == 'note' or item['label'] == 'freq' or item['label'] == 'gain' or item['label'] == 'velocity':
            continue
        elif item['type'] in ('tgroup', 'vgroup', 'hgroup'):
            generated_submenus.append(generateMenu(item['items'], item['label'], False, cur_depth+1))
            submenus.append(item['label'])
        elif item["type"] in ('hslider', 'vslider', 'nentry'):
            menu_data = None
            fine = 0
            unit = ""

            skip = False

            if 'meta' in item.keys():
                for meta in item['meta']:
                    if 'fine' in meta.keys():
                        fine = meta['fine']
                    if 'style' in meta.keys():
                        style = meta['style']
                        menu_data = "{"+style.replace(";", ",").replace("menu", "'menu': ")+"}"
                        menu_data = json.loads(menu_data.replace("'", '"'))
                        if 'menu' not in menu_data.keys():
                            menu_data = None
                    if 'unit' in meta.keys():
                        unit = meta['unit']
                    if 'io' in meta.keys():
                        ioPort = meta['io'].strip()
                        min = item['min']
                        max = item['max']
                        updateAnalog += f"\t\tfi[v]->setParamValue(\"{item['label']}\", (analogRead({ioPort})/(float) 1024)*(((float) {max})-((float) {min}))+((float) {min}));\n"
                        usesAnalog = True
                        skip = True
                    if 'acc' in meta.keys():
                        axis = meta['acc'].strip().split(" ")[0]
                        min = item['min']
                        max = item['max']
                        updateGyro += f"\t\tfi[v]->setParamValue(\"{item['label']}\", (accelerometer_{axis}/(float) 0xFFFF)*(((float) {max})-((float) {min}))+((float) {min}));\n"
                        usesGyro = True
                        skip = True
                    if 'gyr' in meta.keys():
                        axis = meta['gyr'].strip().split(" ")[0]
                        min = item['min']
                        max = item['max']
                        updateGyro += f"\t\tfi[v]->setParamValue(\"{item['label']}\", (gyro_{axis}/(float) 0xFFFF)*(((float) {max})-((float) {min}))+((float) {min}));\n"
                        usesGyro = True
                        skip = True

            if skip:
                continue

            patch += f"\t&t_{variable_name(item['label'])},\n"
            variables += f"float t_{variable_name(item['label'])} = {item['init']};\n"

            if menu_data:
                subprompt = ""
                if cur_depth + 1 > maxdepth:
                    maxdepth = cur_depth + 1

                subprompt += f"prompt* {variable_name(item['label'])}_data[]={{\n"
                for key, val in menu_data['menu'].items():
                    subprompt += f"\tnew menuValue<float>(\"{key}\", {val}),\n"

                subprompt = fix_arduino_menu_direction(subprompt)
                variables = variables + subprompt
                variables = variables[:-2]+"\n};\n"
                menu += f"\tnew Menu::select<float>(\"{item['label']}\", t_{variable_name(item['label'])}, sizeof({variable_name(item['label'])}_data)/sizeof(prompt*), {variable_name(item['label'])}_data,updateFParam, exitEvent, wrapStyle),\n"
            else:
                menu += f"\tnew menuField<float>(t_{variable_name(item['label'])}, \"{item['label']}\", \"{unit}\", {item['min']}, {item['max']}, {item['step']}, {fine}, updateFParam,enterEvent, noStyle),\n"

            updateFunc += f"\t\tfi[v]->setParamValue(\"{item['label']}\", t_{variable_name(item['label'])});\n"

    for label in submenus:
        menu += f"\t&m_{variable_name(label)},\n"

    if top:
        variables += "int t_file_load = 0;\n"
        variables += "int t_file_save = 0;\n"
        menu += ("\tnew menuField<int>(t_file_load, \"load\", \"\", 0, 127, 1, 0, load_from_sdcard, exitEvent, noStyle),\n"
                 "\tnew menuField<int>(t_file_save, \"save\", \"\", 0, 127, 1, 0, save_to_sdcard, exitEvent, noStyle),\n")

        patch = patch[:-2] + "\n};\n"

        updateFunc += "\t}\n\treturn proceed;\n}"
        updateAnalog += "\t}\n}"
        updateGyro += "\t}\n}"
    else:
        menu += '\tnew Exit("<Back"),\n'

    menu = fix_arduino_menu_direction(menu)
    menu = menu[:-2] + "\n};\n"

    menu = "".join(generated_submenus) + menu

    if top:
        menu += f'menuNode &myMenu = *new menuNode("{menulabel}", sizeof(p_{variable_name(menulabel)})/sizeof(prompt*), p_{variable_name(menulabel)});\n'
    else:
        menu += f'menuNode &m_{variable_name(menulabel)} = *new menuNode("{menulabel}", sizeof(p_{variable_name(menulabel)})/sizeof(prompt*), p_{variable_name(menulabel)});\n'

    return menu

with open(f'src{os.sep}FaustInstrument.dsp.json') as f:
    data = json.load(f)

name = data['name']
ui = data['ui'][0]

menu = generateMenu(ui['items'], ui['label'])

defines = f"#define MAX_DEPTH {maxdepth}\n"
if usesAnalog:
    defines += "#define USEANALOG\n"
if usesGyro:
    defines += "#define USEGYRO\n"
