#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include "main.h"
#include "pros/apix.h"
#include "vdml/registry.h"

#define map(value, iMin, iMax, oMin, oMax) ((value - iMin) / (double)(iMax - iMin) * (oMax - oMin) + oMin)
#define expectedSpeed(voltage) ((voltage * 381) / 20000.0)

class Button
{
public:
	lv_style_t * buttonStyleRel = NULL;
	lv_style_t * buttonStylePr = NULL;
	lv_style_t * labelStyle = NULL;
	lv_obj_t * object = NULL;
	lv_obj_t * label = NULL;

	Button(lv_obj_t * parent, lv_coord_t x, lv_coord_t y, lv_coord_t width, lv_coord_t height)
	{
		object = lv_btn_create(parent, NULL);
		lv_obj_set_pos(object, x, y);
		lv_obj_set_size(object, width, height);

		label = lv_label_create(object, NULL);
		lv_label_set_align(label, LV_LABEL_ALIGN_CENTER);
		lv_label_set_recolor(label, true);
		lv_obj_set_style(label, labelStyle);
		lv_label_set_text(label, "");
	}

	void setStyle(lv_color_t colorRel, lv_color_t colorPr, lv_color_t textColor)
	{
		if(buttonStyleRel == NULL) {buttonStyleRel = (lv_style_t *)malloc(sizeof(lv_style_t));lv_style_copy(buttonStyleRel, &lv_style_plain);}
		if(buttonStylePr == NULL) {buttonStylePr = (lv_style_t *)malloc(sizeof(lv_style_t));lv_style_copy(buttonStylePr, &lv_style_plain);}
		if(labelStyle == NULL) {labelStyle = (lv_style_t *)malloc(sizeof(lv_style_t));lv_style_copy(labelStyle, &lv_style_plain);}
		buttonStyleRel->body.main_color = buttonStyleRel->body.grad_color = colorRel;
		buttonStylePr->body.main_color = buttonStylePr->body.grad_color = colorPr;
		labelStyle->text.color = textColor;
		updateStyle();
	}

	void updateStyle()
	{
		lv_btn_set_style(object, LV_BTN_STYLE_REL, buttonStyleRel);
		lv_btn_set_style(object, LV_BTN_STYLE_PR, buttonStylePr);
		lv_obj_set_style(label, labelStyle);
	}

	void setAction(lv_btn_action_t actionType, lv_action_t action) {lv_btn_set_action(object, actionType, action);}

	void setId(uint32_t idNumber = UINT_FAST32_MAX) {lv_obj_set_free_num(object, idNumber);}

	void setTitle(const char * text) {lv_label_set_text(label, text);}
};

class Graph
{
private:
	struct Line
	{
		lv_obj_t * object = NULL;
		lv_style_t * style = NULL;
		lv_point_t * points = NULL;
	};

	lv_style_t * backgroundStyle;
	lv_obj_t * backgroundObject;
	lv_coord_t graphWidth, graphHeight;
	double yMin, yMax, xMin, xMax;
	std::vector<Line> yGuides, xGuides, line;

	Line createLine(lv_color_t color, lv_coord_t width, lv_opa_t opa)
	{
		Line newLine;
		newLine.object = lv_line_create(backgroundObject, NULL);
		newLine.style = (lv_style_t *)malloc(sizeof(lv_style_t));
		lv_style_copy(newLine.style, &lv_style_plain);
		newLine.style->line = {color, width, opa};
		lv_obj_set_style(newLine.object, newLine.style);
		return newLine;
	}

public:
	Graph(lv_obj_t * parent, lv_coord_t x, lv_coord_t y, lv_coord_t graphWidth, lv_coord_t graphHeight, double yMin = -100, double yMax = 100, double xMin = 0, double xMax = 100, lv_color_t backgroundColor = LV_COLOR_WHITE)
		: graphWidth(graphWidth), graphHeight(graphHeight), yMin(yMin), yMax(yMax), xMin(xMin), xMax(xMax)
	{
		backgroundObject = lv_obj_create(parent, NULL);
		lv_obj_set_pos(backgroundObject, x, y);
		lv_obj_set_size(backgroundObject, graphWidth, graphHeight);
		backgroundStyle = (lv_style_t *)malloc(sizeof(lv_style_t));
		lv_style_copy(backgroundStyle, &lv_style_plain);
		backgroundStyle->body.main_color = backgroundStyle->body.grad_color = backgroundColor;
		lv_obj_set_style(backgroundObject, backgroundStyle);
	}

	void addYGuide(lv_coord_t y, lv_color_t color = LV_COLOR_BLACK, lv_coord_t width = 1, lv_opa_t opa = LV_OPA_100)
	{
		Line guideLine = createLine(color, width, opa);
		guideLine.points = (lv_point_t *)std::malloc(2 * sizeof(lv_point_t));
		guideLine.points[0] = {0, (lv_coord_t)map(y, yMin, yMax, graphHeight, 0)};
		guideLine.points[1] = {graphWidth, (lv_coord_t)map(y, yMin, yMax, graphHeight, 0)};
		lv_line_set_points(guideLine.object, guideLine.points, 2);
		yGuides.push_back(guideLine);
	}

	void addXGuide(lv_coord_t x, lv_color_t color = LV_COLOR_BLACK, lv_coord_t width = 1, lv_opa_t opa = LV_OPA_100)
	{
		Line guideLine = createLine(color, width, opa);
		guideLine.points = (lv_point_t *)std::malloc(2 * sizeof(lv_point_t));
		guideLine.points[0] = {(lv_coord_t)map(x, xMin, xMax, 0, graphWidth), 0};
		guideLine.points[1] = {(lv_coord_t)map(x, xMin, xMax, 0, graphWidth), graphHeight};
		lv_line_set_points(guideLine.object, guideLine.points, 2);
		xGuides.push_back(guideLine);
	}

	void add(lv_color_t color = LV_COLOR_BLACK, lv_coord_t width = 1, lv_opa_t opa = LV_OPA_100) {line.push_back(createLine(color, width, opa));}

	void setPoints(int lineIndex, std::vector<int> x, std::vector<int> y)
	{
		if(lineIndex >= line.size() || y.size() < x.size()) return;
		std::free(line[lineIndex].points);
		line[lineIndex].points = (lv_point_t *)std::malloc(x.size() * sizeof(lv_point_t));
		for(int i = 0; i < x.size(); i++) line[lineIndex].points[i] = {(lv_coord_t)map(x[i], xMin, xMax, 0, graphWidth), (lv_coord_t)map(y[i], yMin, yMax, graphHeight, 0)};
		lv_line_set_points(line[lineIndex].object, line[lineIndex].points, x.size());
	}

	void clear(int lineIndex)
	{
		setPoints(lineIndex, {}, {});
	}
};

lv_obj_t * allInfoPage = lv_obj_create(lv_scr_act(), NULL);
Button * box[24];

lv_obj_t * motorInfoPage = lv_obj_create(lv_scr_act(), NULL);
Button motorInfoTitle(motorInfoPage, 0, 0, LV_HOR_RES, 50);
Button motorInfoBackButton(motorInfoPage, 0, 0, 75, 50);
lv_obj_t * motorInfoText;
lv_obj_t * motorInfoSwitch;
lv_style_t bg_style, indic_style, knob_on_style, knob_off_style;
Graph motorInfoGraph(motorInfoPage, 150, 50, LV_HOR_RES - 150, LV_VER_RES - 50, -120, 120);
Button motorInfoRetestButton(motorInfoPage, 0, LV_VER_RES - 50, 150, 50);

lv_obj_t * controllerPage = lv_obj_create(lv_scr_act(), NULL);
Button controllerTitle(controllerPage, 0, 0, LV_HOR_RES, 50);
Button controllerBackButton(controllerPage, 0, 0, 75, 50);
lv_style_t ctrStyleOpen, ctrStyleClosed;
struct
{
	Button * title;
	Button * l2, * l1;
	Button * r2, * r1;
	lv_obj_t * lJoyOuter, * lJoyInner;
	lv_obj_t * rJoyOuter, * rJoyInner;
	Button * up, * right, * down, * left;
	Button * x, * a, * b, * y;

} controller[2];

lv_obj_t * adiPage = lv_obj_create(lv_scr_act(), NULL);
Button adiTitle(adiPage, 0, 0, LV_HOR_RES, 50);
Button adiBackButton(adiPage, 0, 0, 75, 50);
lv_style_t adiPortDisplayStyle;
Button * adiPortTitle[8];
lv_obj_t * adiPortDisplay[8];
Button * adiPortValue[8];
const char * adiName[] = {"A", "B", "C", "D", "E", "F", "G", "H"};

lv_obj_t * infoPage = lv_obj_create(lv_scr_act(), NULL);
Button infoTitle(infoPage, 0, 0, LV_HOR_RES, 50);
Button infoBackButton(infoPage, 0, 0, 75, 50);
lv_obj_t * infoText;

void setButton(Button * button, bool state)
{
	if(state) button->buttonStyleRel = button->buttonStylePr = &ctrStyleClosed;
	else button->buttonStyleRel = button->buttonStylePr = &ctrStyleOpen;
	button->updateStyle();
}

int currentPage = 0;
int motorSelected = 0;

struct TestPoint
{
	int voltage;
	double settleSpeed;
	int settleCurrent;
};

struct TestPointResult
{
	double settleSpeed;
	int settleCurrent;
};

struct PortData
{
	int state = 0;
	long lastPlug = -1;
	pros::c::v5_device_e_t device = pros::c::E_DEVICE_NONE;
	int requestedVoltageValue = 0;
	double acceleration = 0;
	long testingStart = 0;
	int testPoint = 0;
	int testPointStep = 0;
	long testStart = 0;
	long lastReading = 0;
	std::vector<long> time;
	std::vector<int> appliedVoltage;
	std::vector<int> requestedVoltage;
	std::vector<int> current;
	std::vector<int> velocity;
	std::vector<TestPointResult> results;
	long start = 0;
	int coastTime;
	int breakTime;
	bool motorWorking = false;
	bool currentWorking = false;
	double averageScore = 0;
	bool timedOut = false;
	bool breakModeWorking = true;
};

const int maxMotorsRunning = 8;
int testingTimeout = 8000;
int currentMotorsRunning = 0;
PortData portData[21];

int readingInterval = 3;
double errorPercent = 5;
TestPoint testPointList[] = {
	{6000, 117, 70},
	{12000, 237, 160},
	{-6000, -117, 73},
	{-12000, -236, 156},
};

int averageCoastTime = 885;
int averageBreakTime = 196;

TestPointResult testResultSum[4] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
int coastTimeResultSum = 0;
int breakTimeResultSum = 0;
int totalResultCount = 0;

void updateMotorInfo()
{
	lv_obj_set_hidden(motorInfoRetestButton.object, portData[motorSelected].device != pros::c::E_DEVICE_MOTOR);

	std::string a = "Port " + std::to_string(motorSelected + 1);
	if(portData[motorSelected].device == pros::c::E_DEVICE_MOTOR) a += ": Motor";
	if(portData[motorSelected].device == pros::c::E_DEVICE_RADIO) a += ": Radio";
	if(portData[motorSelected].device == pros::c::E_DEVICE_VISION) a += ": Vision";

	if(portData[motorSelected].state >= 100)
	{
		std::stringstream stream;
		stream << std::fixed << std::setprecision(2) << portData[motorSelected].averageScore;
		a += ": " + stream.str() + "%";

		if(portData[motorSelected].timedOut) a += "\n#ff0000 Error: Timed Out#";
		else if(!portData[motorSelected].motorWorking) a += "\n#ff0000 Error: Motor Not Running#";
		else if(!portData[motorSelected].currentWorking) a += "\n#ff0000 Error: Current Reading Problem#";
		else if(!portData[motorSelected].breakModeWorking) a += "\n#ff0000 Error: Motor Brake Not Working#";
	}

	motorInfoTitle.setTitle(a.c_str());

	a = "#008080 Current#\n#000080 Velocity#\n";
	if(lv_sw_get_state(motorInfoSwitch)) a += "#ffa500 Applied Voltage#\n#00ff00 Voltage#\n";

	if(portData[motorSelected].results.size() > 0)
	{
		double ssResult = 0;
		double scResult = 0;
		double cResult = portData[motorSelected].coastTime / (double)averageCoastTime * 100.0 - 100.0;
		double bResult = tanh((averageBreakTime - portData[motorSelected].breakTime) * 0.005) * 100.0;
		if(bResult > 10) bResult = 10;

		if(portData[motorSelected].coastTime == 0) cResult = 0;
		if(portData[motorSelected].breakTime == 0) bResult = 0;

		for(int i = 0; i < portData[motorSelected].results.size(); i++)
		{
			int testPoint = i % (sizeof(testPointList) / sizeof(TestPoint));

			double ssPercent = portData[motorSelected].results[i].settleSpeed / (double)testPointList[testPoint].settleSpeed * 100.0 - 100.0;
			double scPercent = testPointList[testPoint].settleCurrent / (double)(portData[motorSelected].results[i].settleCurrent + 0.0001) * 100.0 - 100.0;

			ssResult += ssPercent;
			scResult += scPercent;
		}

		ssResult /= portData[motorSelected].results.size();
		scResult /= portData[motorSelected].results.size();

		a += "SS: " + std::to_string((int)ssResult) + ", ";
		a += "SC: " + std::to_string((int)scResult) + "\n";
		a += "C: " + std::to_string((int)cResult) + ", ";
		a += "B: " + std::to_string((int)bResult) + "\n";
	}

	if(pros::c::motor_get_temperature(motorSelected + 1) != PROS_ERR) a += "Temp: " + std::to_string((int)pros::c::motor_get_temperature(motorSelected + 1));

	lv_label_set_text(motorInfoText, a.c_str());

	int timeFrame = 1;
	if(portData[motorSelected].time.size() > 2) timeFrame = portData[motorSelected].time.back() - portData[motorSelected].time.front();

	std::vector<int> time, appliedVoltage, requestedVoltage, current, velocity;

	for(int i = 0; i < portData[motorSelected].time.size(); i++)
	{
		time.push_back((portData[motorSelected].time[i] - portData[motorSelected].time.front()) / (double)timeFrame * 100);
		requestedVoltage.push_back(expectedSpeed(portData[motorSelected].requestedVoltage[i]) / 2.5);
		appliedVoltage.push_back(expectedSpeed(portData[motorSelected].appliedVoltage[i]) / 2.5);
		current.push_back(portData[motorSelected].current[i] / 20.0);
		velocity.push_back(portData[motorSelected].velocity[i] / 2.5);
	}

	if(lv_sw_get_state(motorInfoSwitch))
	{
		motorInfoGraph.setPoints(0, time, requestedVoltage);
		motorInfoGraph.setPoints(1, time, appliedVoltage);
	}
	else
	{
		motorInfoGraph.clear(0);
		motorInfoGraph.clear(1);
	}
	motorInfoGraph.setPoints(2, time, current);
	motorInfoGraph.setPoints(3, time, velocity);
}

lv_res_t clickAction(lv_obj_t * btn)
{
    uint32_t i = lv_obj_get_free_num(btn);

	if(i >= 0 && i < 21)
	{
		currentPage = 1;
		motorSelected = i;
		updateMotorInfo();
	}
	if(i == 21) currentPage = 2;
	if(i == 22) currentPage = 3;
	if(i == 23) currentPage = 4;

	if(btn == motorInfoBackButton.object ||
		btn == controllerBackButton.object ||
		btn == adiBackButton.object ||
		btn == infoBackButton.object) currentPage = 0;

	if(btn == motorInfoRetestButton.object && portData[motorSelected].device == pros::c::E_DEVICE_MOTOR)
	{
		if(portData[motorSelected].state >= 3 || portData[motorSelected].state <= 9) currentMotorsRunning--;

		box[motorSelected]->setStyle(LV_COLOR_WHITE, LV_COLOR_WHITE, LV_COLOR_BLACK);

		portData[motorSelected].lastReading = 0;
		portData[motorSelected].time.clear();
		portData[motorSelected].current.clear();
		portData[motorSelected].velocity.clear();
		portData[motorSelected].results.clear();
		portData[motorSelected].coastTime = 0;
		portData[motorSelected].breakTime = 0;

		pros::c::motor_move(motorSelected + 1, 0);
		portData[motorSelected].requestedVoltageValue = 0;
		portData[motorSelected].state = 1;
		portData[motorSelected].motorWorking = false;
		portData[motorSelected].currentWorking = false;
		portData[motorSelected].timedOut = false;
		portData[motorSelected].breakModeWorking = true;

		updateMotorInfo();
	}

    return LV_RES_OK;
}

lv_res_t event_handler(lv_obj_t * obj)
{
	if(currentPage == 1) updateMotorInfo();
	return LV_RES_OK;
}

void opcontrol()
{
	lv_obj_set_style(allInfoPage, &lv_style_plain);
	lv_obj_set_size(allInfoPage, LV_HOR_RES, LV_VER_RES);
	lv_obj_set_style(motorInfoPage, &lv_style_plain);
	lv_obj_set_size(motorInfoPage, LV_HOR_RES, LV_VER_RES);
	lv_obj_set_style(controllerPage, &lv_style_plain);
	lv_obj_set_size(controllerPage, LV_HOR_RES, LV_VER_RES);
	lv_obj_set_style(adiPage, &lv_style_plain);
	lv_obj_set_size(adiPage, LV_HOR_RES, LV_VER_RES);
	lv_obj_set_style(infoPage, &lv_style_plain);
	lv_obj_set_size(infoPage, LV_HOR_RES, LV_VER_RES);

	int blockWidth = LV_HOR_RES / 6;
	int blockHeight = LV_VER_RES / 4;

	for(int i = 0; i < 24; i++)
	{
		int x = (i % 6) * blockWidth;
		int y = roundf(i / 6) * blockHeight;

		box[i] = new Button(allInfoPage, x, y, blockWidth, blockHeight);
		box[i]->setStyle(LV_COLOR_WHITE, LV_COLOR_WHITE, LV_COLOR_BLACK);
		box[i]->setAction(LV_BTN_ACTION_CLICK, clickAction);
		box[i]->setId(i);

		box[i]->labelStyle->text.line_space = -3;
		box[i]->updateStyle();
	}

	box[21]->setTitle("Test\nControl-\nlers");
	box[22]->setTitle("Test\n3-Wire\nPorts");
	box[23]->setTitle("Extra\nInfo");

	motorInfoBackButton.setStyle(LV_COLOR_WHITE, LV_COLOR_WHITE, LV_COLOR_BLACK);
	motorInfoBackButton.setAction(LV_BTN_ACTION_CLICK, clickAction);
	motorInfoBackButton.setId();
	motorInfoBackButton.setTitle(SYMBOL_LEFT);

	motorInfoTitle.setStyle(LV_COLOR_WHITE, LV_COLOR_WHITE, LV_COLOR_BLACK);

	motorInfoText = lv_label_create(motorInfoPage, NULL);
	lv_obj_set_pos(motorInfoText, 3, 50);
	lv_label_set_recolor(motorInfoText, true);
	lv_obj_set_style(motorInfoText, &lv_style_plain);

	lv_style_copy(&bg_style, &lv_style_pretty);
    bg_style.body.radius = LV_RADIUS_CIRCLE;
	bg_style.body.padding.hor = 0;

    lv_style_copy(&indic_style, &lv_style_pretty_color);
    indic_style.body.radius = LV_RADIUS_CIRCLE;
    indic_style.body.main_color = LV_COLOR_MAKE(0x9f, 0xc8, 0xef);
    indic_style.body.grad_color = LV_COLOR_MAKE(0x9f, 0xc8, 0xef);
	indic_style.body.padding.hor = 0;
	indic_style.body.padding.ver = 0;

    lv_style_copy(&knob_off_style, &lv_style_pretty);
    knob_off_style.body.radius = LV_RADIUS_CIRCLE;
    knob_off_style.body.shadow.width = 4;
    knob_off_style.body.shadow.type = LV_SHADOW_BOTTOM;

    lv_style_copy(&knob_on_style, &lv_style_pretty_color);
    knob_on_style.body.radius = LV_RADIUS_CIRCLE;
    knob_on_style.body.shadow.width = 4;
    knob_on_style.body.shadow.type = LV_SHADOW_BOTTOM;

	motorInfoSwitch = lv_sw_create(motorInfoPage, NULL);
	lv_sw_set_style(motorInfoSwitch, LV_SW_STYLE_BG, &bg_style);
    lv_sw_set_style(motorInfoSwitch, LV_SW_STYLE_INDIC, &indic_style);
    lv_sw_set_style(motorInfoSwitch, LV_SW_STYLE_KNOB_ON, &knob_on_style);
    lv_sw_set_style(motorInfoSwitch, LV_SW_STYLE_KNOB_OFF, &knob_off_style);
	lv_obj_set_size(motorInfoSwitch, 60, 40);
	lv_obj_align(motorInfoSwitch, NULL, LV_ALIGN_IN_TOP_RIGHT, -10, 10);
	lv_sw_set_action(motorInfoSwitch, event_handler);

	motorInfoGraph.addYGuide(0);
	motorInfoGraph.add(LV_COLOR_LIME, 2);
	motorInfoGraph.add(LV_COLOR_ORANGE, 2);
	motorInfoGraph.add(LV_COLOR_TEAL, 2);
	motorInfoGraph.add(LV_COLOR_NAVY, 2);

	motorInfoRetestButton.setStyle(LV_COLOR_MAKE(0x00, 0x65, 0xA0), LV_COLOR_MAKE(0x00, 0x65, 0xA0), LV_COLOR_WHITE);
	motorInfoRetestButton.setAction(LV_BTN_ACTION_CLICK, clickAction);
	motorInfoRetestButton.setId();
	motorInfoRetestButton.setTitle("Retest Motor");

	controllerBackButton.setStyle(LV_COLOR_WHITE, LV_COLOR_WHITE, LV_COLOR_BLACK);
	controllerBackButton.setAction(LV_BTN_ACTION_CLICK, clickAction);
	controllerBackButton.setId();
	controllerBackButton.setTitle(SYMBOL_LEFT);

	controllerTitle.setStyle(LV_COLOR_WHITE, LV_COLOR_WHITE, LV_COLOR_BLACK);
	controllerTitle.setTitle("Test Controllers");

	lv_style_copy(&ctrStyleOpen, &lv_style_plain);
	lv_style_copy(&ctrStyleClosed, &lv_style_plain);

	ctrStyleOpen.body.main_color = ctrStyleOpen.body.grad_color = LV_COLOR_WHITE;
	ctrStyleOpen.body.border.width = 1;
	ctrStyleOpen.body.border.color = LV_COLOR_NAVY;
	ctrStyleOpen.body.radius = 100;
	ctrStyleOpen.text.color = LV_COLOR_NAVY;

	ctrStyleClosed.body.main_color = ctrStyleClosed.body.grad_color = LV_COLOR_NAVY;
	ctrStyleClosed.body.border.width = 1;
	ctrStyleClosed.body.border.color = LV_COLOR_NAVY;
	ctrStyleClosed.body.radius = 100;
	ctrStyleClosed.text.color = LV_COLOR_WHITE;

	for(int i = 0; i < 2; i++)
	{
		controller[i].title = new Button(controllerPage, i * LV_HOR_RES * 0.5, 50, LV_HOR_RES * 0.5, 45);
		controller[i].title->setStyle(LV_COLOR_WHITE, LV_COLOR_WHITE, LV_COLOR_BLACK);

		controller[i].l2 = new Button(controllerPage, 5 + i * LV_HOR_RES * 0.5, 95, 50, 25);controller[i].l2->setTitle("L2");
		controller[i].l1 = new Button(controllerPage, 5 + i * LV_HOR_RES * 0.5, 125, 50, 25);controller[i].l1->setTitle("L1");
		controller[i].r2 = new Button(controllerPage, (i + 1) * LV_HOR_RES * 0.5 - 55, 95, 50, 25);controller[i].r2->setTitle("R2");
		controller[i].r1 = new Button(controllerPage, (i + 1) * LV_HOR_RES * 0.5 - 55, 125, 50, 25);controller[i].r1->setTitle("R1");

		controller[i].lJoyOuter = lv_obj_create(controllerPage, NULL);
		lv_obj_set_pos(controller[i].lJoyOuter, 58 + i * LV_HOR_RES * 0.5, 95);
		lv_obj_set_size(controller[i].lJoyOuter, 60, 60);
		lv_obj_set_style(controller[i].lJoyOuter, &ctrStyleOpen);

		controller[i].lJoyInner = lv_obj_create(controller[i].lJoyOuter, NULL);
		lv_obj_set_pos(controller[i].lJoyInner, 25, 25);
		lv_obj_set_size(controller[i].lJoyInner, 10, 10);
		lv_obj_set_style(controller[i].lJoyInner, &ctrStyleClosed);

		controller[i].rJoyOuter = lv_obj_create(controllerPage, NULL);
		lv_obj_set_pos(controller[i].rJoyOuter, (i + 1) * LV_HOR_RES * 0.5 - 118, 95);
		lv_obj_set_size(controller[i].rJoyOuter, 60, 60);
		lv_obj_set_style(controller[i].rJoyOuter, &ctrStyleOpen);

		controller[i].rJoyInner = lv_obj_create(controller[i].rJoyOuter, NULL);
		lv_obj_set_pos(controller[i].rJoyInner, 25, 25);
		lv_obj_set_size(controller[i].rJoyInner, 10, 10);
		lv_obj_set_style(controller[i].rJoyInner, &ctrStyleClosed);

		controller[i].up = new Button(controllerPage, 40 + i * LV_HOR_RES * 0.5, 155, 30, 30);controller[i].up->setTitle(SYMBOL_UP);
		controller[i].right = new Button(controllerPage, 65 + i * LV_HOR_RES * 0.5, 180, 30, 30);controller[i].right->setTitle(SYMBOL_RIGHT);
		controller[i].down = new Button(controllerPage, 40 + i * LV_HOR_RES * 0.5, 205, 30, 30);controller[i].down->setTitle(SYMBOL_DOWN);
		controller[i].left = new Button(controllerPage, 15 + i * LV_HOR_RES * 0.5, 180, 30, 30);controller[i].left->setTitle(SYMBOL_LEFT);

		controller[i].x = new Button(controllerPage, (i + 1) * LV_HOR_RES * 0.5 - 70, 155, 30, 30);controller[i].x->setTitle("X");
		controller[i].a = new Button(controllerPage, (i + 1) * LV_HOR_RES * 0.5 - 45, 180, 30, 30);controller[i].a->setTitle("A");
		controller[i].b = new Button(controllerPage, (i + 1) * LV_HOR_RES * 0.5 - 70, 205, 30, 30);controller[i].b->setTitle("B");
		controller[i].y = new Button(controllerPage, (i + 1) * LV_HOR_RES * 0.5 - 95, 180, 30, 30);controller[i].y->setTitle("Y");
	}

	adiBackButton.setStyle(LV_COLOR_WHITE, LV_COLOR_WHITE, LV_COLOR_BLACK);
	adiBackButton.setAction(LV_BTN_ACTION_CLICK, clickAction);
	adiBackButton.setId();
	adiBackButton.setTitle(SYMBOL_LEFT);

	adiTitle.setStyle(LV_COLOR_WHITE, LV_COLOR_WHITE, LV_COLOR_BLACK);
	adiTitle.setTitle("Test 3-Wire Ports");

	lv_style_copy(&adiPortDisplayStyle, &lv_style_plain);
	adiPortDisplayStyle.body.main_color = adiPortDisplayStyle.body.grad_color = LV_COLOR_NAVY;

	int adiPortWidth = LV_HOR_RES / 8;

	for(int i = 0; i < 8; i++)
	{
		pros::c::adi_pin_mode(i + 1, INPUT_ANALOG);
		adiPortTitle[i] = new Button(adiPage, i * adiPortWidth, 50, adiPortWidth, 45);
		adiPortTitle[i]->setStyle(LV_COLOR_WHITE, LV_COLOR_WHITE, LV_COLOR_BLACK);
		std::string a = "Port\n" + (std::string)adiName[i];
		adiPortTitle[i]->setTitle(a.c_str());

		adiPortDisplay[i] = lv_obj_create(adiPage, NULL);
		lv_obj_set_style(adiPortDisplay[i], &adiPortDisplayStyle);

		adiPortValue[i] = new Button(adiPage, i * adiPortWidth, LV_VER_RES - 45, adiPortWidth, 45);
		adiPortValue[i]->setStyle(LV_COLOR_WHITE, LV_COLOR_WHITE, LV_COLOR_BLACK);
	}

	infoBackButton.setStyle(LV_COLOR_WHITE, LV_COLOR_WHITE, LV_COLOR_BLACK);
	infoBackButton.setAction(LV_BTN_ACTION_CLICK, clickAction);
	infoBackButton.setId();
	infoBackButton.setTitle(SYMBOL_LEFT);

	infoTitle.setStyle(LV_COLOR_WHITE, LV_COLOR_WHITE, LV_COLOR_BLACK);
	infoTitle.setTitle("Extra Info");

	infoText = lv_label_create(infoPage, NULL);
	lv_obj_set_pos(infoText, 3, 50);
	lv_label_set_recolor(infoText, true);
	lv_obj_set_style(infoText, &lv_style_plain);

	while(true)
	{
		if(currentPage == 0) lv_obj_set_parent(allInfoPage, lv_scr_act());
		if(currentPage == 1) lv_obj_set_parent(motorInfoPage, lv_scr_act());
		if(currentPage == 2) lv_obj_set_parent(controllerPage, lv_scr_act());
		if(currentPage == 3) lv_obj_set_parent(adiPage, lv_scr_act());
		if(currentPage == 4) lv_obj_set_parent(infoPage, lv_scr_act());

		for(int i = 0; i < 21; i++)
		{
			pros::c::v5_device_e_t port = pros::c::registry_get_plugged_type(i);

			std::string a = "";
			if(portData[i].state >= 100 && (portData[i].timedOut || !portData[i].motorWorking || !portData[i].currentWorking || !portData[i].breakModeWorking))
				a += "    " + std::to_string(i + 1) + " " + SYMBOL_WARNING + "\n";
			else a += std::to_string(i + 1) + "\n";
			if(port == pros::c::E_DEVICE_MOTOR) a += "Motor";
			if(port == pros::c::E_DEVICE_RADIO) a += "Radio";
			if(port == pros::c::E_DEVICE_VISION) a += "Vision";
			a += "\n";
			if(portData[i].state >= 100)
			{
				if(portData[i].timedOut) a += "TO ERR";
				else if(!portData[i].motorWorking) a += "NR ERR";
				else if(!portData[i].currentWorking) a += "C ERR";
				else if(!portData[i].breakModeWorking) a += "B ERR";
				else
				{
					std::stringstream stream;
					stream << std::fixed << std::setprecision(2) << portData[i].averageScore;
					a += stream.str() + "%";
				}
			}

			box[i]->setTitle(a.c_str());

			if(port != portData[i].device && currentPage == 1) updateMotorInfo();

			if(port == pros::c::E_DEVICE_MOTOR)
			{
				if(portData[i].state == 0)
				{
					portData[i].lastPlug = pros::millis();
					portData[i].device = port;
					portData[i].state++;
				}
				if(portData[i].state == 1
					&& pros::millis() - portData[i].lastPlug > 150
					&& fabs(pros::c::motor_get_actual_velocity(i + 1)) < 5) portData[i].state++;
				if(portData[i].state == 2 && currentMotorsRunning < maxMotorsRunning)
				{
					currentMotorsRunning++;
					portData[i].state++;
					portData[i].testPoint = 0;
					portData[i].testPointStep = 0;
					portData[i].testingStart = pros::millis();
					portData[i].start = pros::millis();
					portData[i].testStart = pros::millis();
					portData[i].motorWorking = false;
					portData[i].currentWorking = false;
				}
				if(portData[i].state == 3)
				{
					int power = testPointList[portData[i].testPoint].voltage;

					pros::c::motor_move_voltage(i + 1, power);
					portData[i].requestedVoltageValue = power;

					if(fabs(pros::c::motor_get_actual_velocity(i + 1)) > 10) portData[i].motorWorking = true;

					if(pros::millis() - portData[i].start > 1000 && !portData[i].motorWorking)
					{
						portData[i].state = 8;
						portData[i].testPoint = 0;
						portData[i].testPointStep = 0;
					}

					if(fabs(portData[i].acceleration) > 500) portData[i].testPointStep = 1;
					if(fabs(portData[i].acceleration) < 250 && portData[i].testPointStep == 1) {portData[i].testPointStep = 2;portData[i].testStart = pros::millis();}
					if(fabs(portData[i].acceleration) > 300 && portData[i].testPointStep == 2) portData[i].testPointStep = 1;
					if(pros::millis() - portData[i].testStart > 100 && portData[i].testPointStep == 2)
					{
						portData[i].testPoint++;
						portData[i].results.push_back({pros::c::motor_get_actual_velocity(i + 1), pros::c::motor_get_current_draw(i + 1)});
						portData[i].testPointStep = 0;

						if(portData[i].testPoint >= sizeof(testPointList) / sizeof(TestPoint))
						{
							currentMotorsRunning--;
							pros::c::motor_move(i + 1, 0);
							portData[i].state++;
							portData[i].testPointStep = 0;
							if(currentPage == 1 && motorSelected == i) updateMotorInfo();
						}
					}
				}
				if(portData[i].state == 4)
				{
					pros::c::motor_move_voltage(i + 1, 12000);
					portData[i].requestedVoltageValue = 12000;

					if(fabs(portData[i].acceleration) > 500) portData[i].testPointStep = 1;
					if(fabs(portData[i].acceleration) < 250 && portData[i].testPointStep == 1) {portData[i].testPointStep = 2;portData[i].testStart = pros::millis();}
					if(fabs(portData[i].acceleration) > 300 && portData[i].testPointStep == 2) portData[i].testPointStep = 1;
					if(pros::millis() - portData[i].testStart > 100 && portData[i].testPointStep == 2)
					{
						portData[i].start = pros::millis();
						pros::c::motor_set_brake_mode(i + 1, pros::E_MOTOR_BRAKE_COAST);
						pros::c::motor_move_voltage(i + 1, 0);
						portData[i].requestedVoltageValue = 0;
						portData[i].state++;
						portData[i].testPointStep = 0;
					}
				}
				if(portData[i].state == 5)
				{
					if(std::fabs(pros::c::motor_get_actual_velocity(i + 1)) < 5)
					{
						portData[i].coastTime = pros::millis() - portData[i].start;
						portData[i].state++;
					}
				}
				if(portData[i].state == 6)
				{
					pros::c::motor_move_voltage(i + 1, 12000);
					portData[i].requestedVoltageValue = 12000;

					if(fabs(portData[i].acceleration) > 500) portData[i].testPointStep = 1;
					if(fabs(portData[i].acceleration) < 250 && portData[i].testPointStep == 1) {portData[i].testPointStep = 2;portData[i].testStart = pros::millis();}
					if(fabs(portData[i].acceleration) > 300 && portData[i].testPointStep == 2) portData[i].testPointStep = 1;
					if(pros::millis() - portData[i].testStart > 100 && portData[i].testPointStep == 2)
					{
						portData[i].start = pros::millis();
						pros::c::motor_set_brake_mode(i + 1, pros::E_MOTOR_BRAKE_BRAKE);
						pros::c::motor_move_voltage(i + 1, 0);
						portData[i].requestedVoltageValue = 0;
						portData[i].state++;
						portData[i].testPointStep = 0;
					}
				}
				if(portData[i].state == 7)
				{
					if(std::fabs(pros::c::motor_get_actual_velocity(i + 1)) < 5)
					{
						portData[i].breakTime = pros::millis() - portData[i].start;
						portData[i].state = 10;
					}
				}
				if(portData[i].state == 8)
				{
					portData[i].start = pros::millis();
					if(portData[i].requestedVoltageValue <= 0)
					{
						pros::c::motor_move_voltage(i + 1, 12000);
						portData[i].requestedVoltageValue = 12000;
					}
					else
					{
						pros::c::motor_move_voltage(i + 1, -12000);
						portData[i].requestedVoltageValue = -12000;
					}
					portData[i].state++;
				}
				if(portData[i].state == 9)
				{
					if(fabs(pros::c::motor_get_actual_velocity(i + 1)) > 10)
					{
						portData[i].motorWorking = true;
						portData[i].testingStart = pros::millis();
						portData[i].state = 3;
					}

					if(pros::millis() - portData[i].start > 1000 && !portData[i].motorWorking)
					{
						portData[i].state = 10;
					}
				}
				if(portData[i].state == 10)
				{
					pros::c::motor_move_voltage(i + 1, 0);

					double totalScore = 0;
					int totalScoreValues = 0;

					for(int a = 0; a < portData[i].results.size(); a++)
					{
						int testPoint = a % (sizeof(testPointList) / sizeof(TestPoint));

						double ssPercent = portData[i].results[a].settleSpeed / (double)testPointList[testPoint].settleSpeed * 100.0 - 100.0;
						double scPercent = testPointList[testPoint].settleCurrent / (double)(portData[i].results[a].settleCurrent + 0.0001) * 100.0 - 100.0;

						totalScore += ssPercent;
						totalScore += scPercent;
						totalScoreValues += 2;

						testResultSum[a].settleSpeed += portData[i].results[a].settleSpeed;
						testResultSum[a].settleCurrent += portData[i].results[a].settleCurrent;
					}

					double bPercent = tanh((averageBreakTime - portData[i].breakTime) * 0.005) * 100.0;
					if(bPercent > 10) bPercent = 10;

					totalScore += portData[i].coastTime / (double)averageCoastTime * 100.0 - 100.0;
					totalScoreValues++;

					if(bPercent < -60) portData[i].breakModeWorking = false;
					else
					{
						totalScore += tanh(abs(averageBreakTime - portData[i].breakTime) * 0.005) * -100.0;
						totalScoreValues++;
					}

					coastTimeResultSum += portData[i].coastTime;
					breakTimeResultSum += portData[i].breakTime;
					totalResultCount++;

					portData[i].averageScore = totalScore / totalScoreValues;

					if(portData[i].averageScore < -40 || !portData[i].motorWorking || !portData[i].currentWorking || portData[i].timedOut || !portData[i].breakModeWorking)
					{
						portData[i].state = 102;
						box[i]->setStyle(LV_COLOR_RED, LV_COLOR_RED, LV_COLOR_WHITE);
					}
					else if(portData[i].averageScore < -35)
					{
						portData[i].state = 101;
						box[i]->setStyle(LV_COLOR_ORANGE, LV_COLOR_ORANGE, LV_COLOR_WHITE);
					}
					else
					{
						portData[i].state = 100;
						box[i]->setStyle(LV_COLOR_GREEN, LV_COLOR_GREEN, LV_COLOR_WHITE);
					}

					if(currentPage == 1 && motorSelected == i) updateMotorInfo();
				}

				if(pros::millis() - portData[i].testingStart > testingTimeout && portData[i].state >= 3 && portData[i].state <= 9)
				{
					portData[i].state = 10;
					portData[i].timedOut = true;
				}

				if(portData[i].state >= 3 && portData[i].state <= 9)
				{
					if(abs(pros::c::motor_get_current_draw(i + 1)) > 10) portData[i].currentWorking = true;
				}

				if(pros::millis() - portData[i].lastReading > readingInterval && portData[i].state >= 3 && portData[i].state <= 9)
				{
					portData[i].time.push_back(pros::millis());
					portData[i].appliedVoltage.push_back(pros::c::motor_get_voltage(i + 1));
					portData[i].requestedVoltage.push_back(portData[i].requestedVoltageValue);
					portData[i].current.push_back(pros::c::motor_get_current_draw(i + 1));
					if(portData[i].velocity.size() == 0) portData[i].velocity.push_back(pros::c::motor_get_actual_velocity(i + 1));
					else portData[i].velocity.push_back(portData[i].velocity.end()[-1] * 0.7 + pros::c::motor_get_actual_velocity(i + 1) * 0.3);

					int velocityChange = portData[i].velocity.end()[-1] - portData[i].velocity.end()[-2];
					int timeChange = portData[i].time.end()[-1] - portData[i].time.end()[-2];
					if(portData[i].velocity.size() > 2) portData[i].acceleration = portData[i].acceleration * 0.7 + velocityChange * 1000.0 / timeChange * 0.3;

					portData[i].lastReading = pros::millis();
					if(currentPage == 1 && motorSelected == i) updateMotorInfo();
				}
			}
			else if(port == pros::c::E_DEVICE_RADIO) portData[i].device = port;
			else if(port == pros::c::E_DEVICE_VISION) portData[i].device = port;
			else if(portData[i].state != 0 && portData[i].device == pros::c::E_DEVICE_MOTOR)
			{
				if(portData[i].state >= 3 || portData[i].state <= 9) currentMotorsRunning--;

				box[i]->setStyle(LV_COLOR_WHITE, LV_COLOR_WHITE, LV_COLOR_BLACK);

				portData[i].lastReading = 0;
				portData[i].time.clear();
				portData[i].current.clear();
				portData[i].velocity.clear();
				portData[i].results.clear();
				portData[i].coastTime = 0;
				portData[i].breakTime = 0;

				pros::c::motor_move(i + 1, 0);
				portData[i].requestedVoltageValue = 0;
				portData[i].device = port;
				portData[i].state = 0;
				portData[i].motorWorking = false;
				portData[i].currentWorking = false;
				portData[i].timedOut = false;
				portData[i].breakModeWorking = true;

				if(currentPage == 1 && motorSelected == i) updateMotorInfo();
			}
			else portData[i].device = port;
		}

		for(int i = 0; i < 2; i++)
		{
			pros::controller_id_e_t id = i == 0 ? pros::E_CONTROLLER_MASTER : pros::E_CONTROLLER_PARTNER;
			bool connected = pros::c::controller_is_connected(id);

			std::string a = (i == 0 ? "Master" : "Partner") + (std::string)" Controller" + "\n";
			if(connected) a += "#00FF00 Connected#";
			else a += "#FF0000 Not Connected#";
			controller[i].title->setTitle(a.c_str());

			setButton(controller[i].l2, pros::c::controller_get_digital(id, pros::E_CONTROLLER_DIGITAL_L2));
			setButton(controller[i].l1, pros::c::controller_get_digital(id, pros::E_CONTROLLER_DIGITAL_L1));
			setButton(controller[i].r2, pros::c::controller_get_digital(id, pros::E_CONTROLLER_DIGITAL_R2));
			setButton(controller[i].r1, pros::c::controller_get_digital(id, pros::E_CONTROLLER_DIGITAL_R1));

			setButton(controller[i].up, pros::c::controller_get_digital(id, pros::E_CONTROLLER_DIGITAL_UP));
			setButton(controller[i].right, pros::c::controller_get_digital(id, pros::E_CONTROLLER_DIGITAL_RIGHT));
			setButton(controller[i].down, pros::c::controller_get_digital(id, pros::E_CONTROLLER_DIGITAL_DOWN));
			setButton(controller[i].left, pros::c::controller_get_digital(id, pros::E_CONTROLLER_DIGITAL_LEFT));

			setButton(controller[i].x, pros::c::controller_get_digital(id, pros::E_CONTROLLER_DIGITAL_X));
			setButton(controller[i].a, pros::c::controller_get_digital(id, pros::E_CONTROLLER_DIGITAL_A));
			setButton(controller[i].b, pros::c::controller_get_digital(id, pros::E_CONTROLLER_DIGITAL_B));
			setButton(controller[i].y, pros::c::controller_get_digital(id, pros::E_CONTROLLER_DIGITAL_Y));

			lv_obj_set_pos(controller[i].lJoyInner, map(pros::c::controller_get_analog(id, pros::E_CONTROLLER_ANALOG_LEFT_X), -127, 127, 0, 50),
				map(pros::c::controller_get_analog(id, pros::E_CONTROLLER_ANALOG_LEFT_Y), -127, 127, 50, 0));
			lv_obj_set_pos(controller[i].rJoyInner, map(pros::c::controller_get_analog(id, pros::E_CONTROLLER_ANALOG_RIGHT_X), -127, 127, 0, 50),
				map(pros::c::controller_get_analog(id, pros::E_CONTROLLER_ANALOG_RIGHT_Y), -127, 127, 50, 0));
		}

		for(int i = 0; i < 8; i++)
		{

			int portValue = pros::c::adi_analog_read(i + 1);
			int displayHeight = map(portValue, 0, 4095, 0, LV_VER_RES - 160);

			lv_obj_set_pos(adiPortDisplay[i], (i + 0.5) * (LV_HOR_RES / 8) - 10, 105 + (LV_VER_RES - 160) - displayHeight);
			lv_obj_set_size(adiPortDisplay[i], 20, displayHeight);

			std::string a = std::to_string(portValue) + "\n/4095";
			adiPortValue[i]->setTitle(a.c_str());
		}

		if(totalResultCount > 0)
		{
			std::string a = "";

			for(int i = 0; i < 4; i++) a += "SS" + std::to_string(i + 1) + ": " + std::to_string((int)testResultSum[i].settleSpeed / totalResultCount) + ", ";
			a += "\n";
			for(int i = 0; i < 4; i++) a += "SC" + std::to_string(i + 1) + ": " + std::to_string(testResultSum[i].settleCurrent / totalResultCount) + ", ";
			a += "\n";
			a += "C: " + std::to_string(coastTimeResultSum / totalResultCount) + "\n";
			a += "B: " + std::to_string(breakTimeResultSum / totalResultCount) + "\n";

			lv_label_set_text(infoText, a.c_str());
		}

		pros::delay(3);
	}
}
