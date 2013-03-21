#include "MobileStream.h"

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

BufferedStream::Buffer __MobileStream__rxBuffer;
BufferedStream::Buffer __MobileStream__txBuffer;

MobileStream::ParseState parseState;

const prog_char C_INIT0[] PROGMEM = "\r\nATV1 E0 X1 S0=0 +CMEE=0";

// CONFIGURATION TO DO
const prog_char C_CONFIGURATION1[] PROGMEM = "AT+CGEREP=2,0;+CIURC=0;+CFGRI=1;+CIPHEAD=1;+CIPSPRT=1;+CIPCSGP=1,\"m2m.t-mobile.com\"";
const prog_char C_START1[] PROGMEM = "AT+CSTT";
const prog_char C_START2[] PROGMEM = "AT+CIICR";
const prog_char C_START3[] PROGMEM = "AT+CIFSR";
// CONFIGURATION TO DO
const prog_char C_DATA_CONNECTION[] PROGMEM = "AT+CIPSTART=\"udp\",\"174.109.85.205\",\"14550\"";
const prog_char C_RESET[] PROGMEM = "AT+CFUN=1,1";
const prog_char C_RESET_IP[] PROGMEM = "AT+CIPSHUT";

const prog_char R_OK[] PROGMEM = "OK";
const prog_char R_ERROR[] PROGMEM = "ERROR";
const prog_char R_SHUT_OK[] PROGMEM = "SHUT OK";
const prog_char R_ANYTHING[] PROGMEM = "";

const prog_char R_RDY[] PROGMEM = "RDY";
const prog_char R_ANY_CFUN[] PROGMEM = "+CFUN: ?";
const prog_char R_CGREG_1[] PROGMEM = "+CGREG: 1";
const prog_char R_CGREG_3[] PROGMEM = "+CGREG: 3";
const prog_char R_CGREG_5[] PROGMEM = "+CGREG: 5";
const prog_char R_IPD[] PROGMEM = "+IPD,";
const prog_char R_CONNECT_OK[] PROGMEM = "CONNECT OK";
const prog_char R_ALREADY_CONNECT[] PROGMEM = "ALREADY CONNECT";
const prog_char R_SEND_OK[] PROGMEM = "SEND OK";
const prog_char R_SEND_FAIL[] PROGMEM = "SEND FAIL";
//const prog_char R_[] PROGMEM = "";

const MobileStream::Transition MobileStream::T_INITIAL[] = { Transition(R_OK,
		&S_CONFIGURATION1), Transition(R_ERROR, &S_RESET) };

const MobileStream::CommandState MobileStream::S_INITIAL =
		MobileStream::CommandState(C_INIT0, 2, T_INITIAL);

const MobileStream::Transition MobileStream::T_RESET[] = { Transition(R_OK,
		&S_INITIAL), Transition(R_ERROR, &S_INITIAL) };

const MobileStream::CommandState MobileStream::S_RESET =
		MobileStream::CommandState(C_RESET, 2, T_RESET);

const MobileStream::Transition MobileStream::T_RESET_IP[] = { Transition(
		R_SHUT_OK, &S_CONFIGURATION1), Transition(R_ERROR, &S_CONFIGURATION1) };

const MobileStream::CommandState MobileStream::S_RESET_IP =
		MobileStream::CommandState(C_RESET_IP, 2, T_RESET_IP);

const MobileStream::DeadState MobileStream::S_DATA_CONNECTION_OPENING =
		MobileStream::DeadState();
const MobileStream::DeadState MobileStream::S_DATA_CONNECTION_OPEN =
		MobileStream::DeadState();

const MobileStream::Transition MobileStream::T_DATA_CONNECTION[] = { Transition(
		R_OK, &S_DATA_CONNECTION_OPENING), Transition(R_ERROR, &S_INITIAL) };

const MobileStream::CommandState MobileStream::S_DATA_CONNECTION =
		MobileStream::CommandState(C_DATA_CONNECTION, 2, T_DATA_CONNECTION);

const MobileStream::Transition MobileStream::T_START3[] = { Transition(
		R_ANYTHING, &S_DATA_CONNECTION), Transition(R_ERROR, &S_RESET_IP) };

const MobileStream::CommandState MobileStream::S_START3 =
		MobileStream::CommandState(C_START3, 2, T_START3);

const MobileStream::Transition MobileStream::T_START2[] = { Transition(R_OK,
		&S_START3), Transition(R_ERROR, &S_RESET_IP) };

const MobileStream::CommandState MobileStream::S_START2 =
		MobileStream::CommandState(C_START2, 2, T_START2);

const MobileStream::Transition MobileStream::T_START1[] = { Transition(R_OK,
		&S_START2), Transition(R_ERROR, &S_RESET_IP) };

const MobileStream::CommandState MobileStream::S_START1 =
		MobileStream::CommandState(C_START1, 2, T_START1);

const MobileStream::Transition MobileStream::T_CONFIGURATION1[] = { Transition(
		R_OK, &S_START1), Transition(R_ERROR, &S_RESET_IP) };

const MobileStream::CommandState MobileStream::S_CONFIGURATION1 =
		MobileStream::CommandState(C_CONFIGURATION1, 2, T_CONFIGURATION1);

const MobileStream::Transition MobileStream::T_DATA_TRANSMISSION[] = {
		Transition(R_SEND_OK, &S_DATA_CONNECTION_OPEN), 
		Transition(R_SEND_FAIL, &S_DATA_CONNECTION_OPEN), 
		Transition(R_ERROR, &S_DATA_CONNECTION_OPEN) };

const MobileStream::DataTransmissionState MobileStream::S_DATA_TRANSMISSION =
		MobileStream::DataTransmissionState(3, T_DATA_TRANSMISSION);

const MobileStream::DataReceptionState MobileStream::S_DATA_RECEPTION =
		MobileStream::DataReceptionState();

/*
 * Responses to URCs (Unsolicited Return Codes) from modem.
 */
const MobileStream::Transition MobileStream::URCTransitions[] = {
		MobileStream::Transition(R_RDY, &S_INITIAL), MobileStream::Transition(
				R_ANY_CFUN, &S_INITIAL), MobileStream::Transition(R_CGREG_1,
				&S_INITIAL), MobileStream::Transition(R_CGREG_3, &S_INITIAL),
		MobileStream::Transition(R_CGREG_5, &S_INITIAL),
		MobileStream::Transition(R_CONNECT_OK, &S_DATA_CONNECTION_OPEN),
		MobileStream::Transition(R_ALREADY_CONNECT, &S_RESET_IP),
		MobileStream::Transition(R_SEND_OK, &S_DATA_CONNECTION_OPEN),
		MobileStream::Transition(R_SEND_FAIL, &S_DATA_CONNECTION_OPEN) };

const uint8_t MobileStream::numURCTransitions =
		sizeof(MobileStream::URCTransitions) / sizeof(MobileStream::Transition);

/*
 * URC interrupts come as <CR><LF>text data....
 * These cannot be delimited by trailing <CR><LF>
 */
const MobileStream::Transition MobileStream::URCInterrupts[] = {
		MobileStream::Transition(R_IPD, &S_DATA_RECEPTION) };

const uint8_t MobileStream::numURCInterrupts =
		sizeof(MobileStream::URCInterrupts) / sizeof(MobileStream::Transition);

MobileStream::MobileStream(BetterStream* modem) :
		modem(modem), userdata(&__MobileStream__rxBuffer,
				&__MobileStream__txBuffer), parseState() {
	userdata.set_blocking_writes(false);
}

MobileStream::DecisionState::DecisionState() :
		numTransitions(0), transitions(NULL) {
}

MobileStream::DecisionState::DecisionState(size_t numTransitions,
		Transition const * transitions) :
		numTransitions(numTransitions), transitions(transitions) {
}

/*
 * Search for an applicable transition. 
 */
const MobileStream::State* MobileStream::DecisionState::match(
		const ParseState& parseState) const {
	uint8_t i;
	for (i = 0; i < numTransitions; i++) {
		if (parseState.match(transitions[i].token)) {
			return transitions[i].state;
		}
	}
	return NULL;
}

/*
 * Receive response message.
 */
void MobileStream::DecisionState::receiveResponseLine(MobileStream* const outer,
		ParseState& parseState) const {
	int data;
	size_t i, j;

	if (parseState.responseTimeout++ >= 50 * 20) {
		parseState.progress = P_START;
		parseState.responseTimeout = 0;
		i = outer->modem->txspace();
		// The most common reason getting stuck is a AT+CIPDATA that
		// has not received all its data. Just give it some dummy data
		// instead.
		for (j=0; j<i; j++) {
			outer->modem->write(' ');
		}
	}

	// If there is no (more) data, quit.
	if ((data = outer->modem->read()) < 0) {
		return;
	}

	/*
	 *  This will accept one or more CR/LF chars as a response header,
	 *  and consume them all.
	 */
	if (data == '\r' || data == '\n') {
		if (parseState.progress == P_SAW_INITIAL_WS) {
			parseState.progress = P_BUFFERING_RESPONSE_LINE;
		} else if (parseState.progress == P_WAITING_INITIAL_WS) {
			parseState.progress = P_SAW_INITIAL_WS;
		} else if (parseState.progress == P_BUFFERING_RESPONSE_LINE) {
			parseState.progress = P_SAW_TRAILING_WS;
		} else if (parseState.progress == P_SAW_TRAILING_WS) {
			parseState.progress = P_RESPONSE_LINE_COMPLETE;
		}
		return; // throw away whitespace.
	} else {
		if (parseState.progress == P_SAW_TRAILING_WS) {
			// ANY char after first trailing whitespace terminates the response.
			parseState.progress = P_RESPONSE_LINE_COMPLETE;
		}
		if (parseState.progress == P_SAW_INITIAL_WS) {
			parseState.progress = P_BUFFERING_RESPONSE_LINE;
		}
		if (parseState.progress == P_BUFFERING_RESPONSE_LINE) {
			parseState.add((uint8_t) data);
		}
	}
}

const MobileStream::State* MobileStream::DecisionState::findSuccessor(
		MobileStream* const outer, ParseState& parseState) const {
	const State* result = match(parseState);
	// States switched to by normal response matching are never immediate but just deferred.
	if (result == NULL) {
		// Try if the response is really no response at all, but a known URC.
		// Unfortunately, the SIM900 does not hold back URCs between the start of an AT command
		// and the end of its response so we can expect them almost any time!!
		result = outer->matchURCs(parseState);
	}
	return result;
}

void MobileStream::DecisionState::task(MobileStream* const outer,
		ParseState& parseState) const {
	// When the command has been sent, we switch to waiting for <CR><LF>response<CR><LF>...
	// When returning from URC "interrupts" aka receiving data, we wait for the original RC still outstanding.
	if (parseState.progress == P_RETURNED_FROM_INTERRUPT
			|| parseState.progress == P_COMMAND_SENT) {
		parseState.progress = P_WAITING_INITIAL_WS;
	}

	if (parseState.progress > P_COMMAND_SENT) {
		receiveResponseLine(outer, parseState);
		if (parseState.progress == P_RESPONSE_LINE_COMPLETE) {
			const State* successor = findSuccessor(outer, parseState);
			if (successor == NULL) {
				// We have a complete line but no known code or even URC matched. Panic. Restart state.
				parseState.beginState(parseState.state);
				parseState.responseBuffer[parseState.bufptr] = 0;
				parseState.progress = P_START;
			} else {
				// Found successor, apply that.
				parseState.beginState(successor);
			}
		} else if (parseState.progress == P_BUFFERING_RESPONSE_LINE) {
			const State* interrupt = outer->matchInterrupts(parseState);
			if (interrupt != NULL) {
				parseState.saveState();
				parseState.beginState(interrupt);
				return;
			}
		}
	}
}

MobileStream::CommandState::CommandState(const prog_char* command,
		size_t numTransitions, Transition const * transitions) :
		DecisionState(numTransitions, transitions), _command(
				(prog_char_t*) command) {
}

MobileStream::CommandState::CommandState(const prog_char_t* command,
		size_t numTransitions, Transition const * transitions) :
		DecisionState(numTransitions, transitions), _command(
				command) {
}

void MobileStream::CommandState::task(MobileStream* const outer,
		ParseState& parseState) const {
	if (parseState.progress == P_RETURNED_FROM_INTERRUPT) {
		parseState.progress = P_START;
	}

	if (parseState.progress == P_START) {
		// Make sure there is space for the command and a trailing CR.
		if ((unsigned int) (outer->modem->txspace()) > strlen_P(_command)) {
			outer->modem->print_P(_command);
			outer->modem->print_P(PSTR("\r\n"));
			parseState.progress = P_COMMAND_SENT;
			parseState.responseTimeout = 0;
		}
	}
	DecisionState::task(outer, parseState);
}

void MobileStream::DeadState::task(MobileStream* const outer,
		ParseState& parseState) const {
	int numBytes;

	// Dead states never send any commands, so skip to waiting for whitespace from modem.
	if (parseState.progress == P_START) {
		parseState.progress = P_WAITING_INITIAL_WS;
	}

	// See if user application wanted to transmit data, and in that case do it.
	// A transmission is started only when an URC reception is not in progress. 
	// Because the GCS (currently) buffers data as whole MAVLink messages at a time, it is simple
	// to get whole messages i UDP package: Any data waiting to be sent is one or more complete
	// MAVLink messages.
	// TODO: Not all dead states should support transmit.

	// Balance between prioritizing input and prioritizing output. Mission Planner sends so few messages that we barely hear them
	// - QGroundStation sends so many that we have to ignore it sometimes...

	if ((parseState.progress == P_WAITING_INITIAL_WS || parseState.progress == P_RETURNED_FROM_INTERRUPT) && this == &S_DATA_CONNECTION_OPEN) {
		if ((numBytes = outer->userdata.backend_available())) {
			parseState.numTxBytes = (size_t) numBytes;
			parseState.beginState(&S_DATA_TRANSMISSION);
			// Switch immediately to transmit state;
			return;
		}
	}

	DecisionState::task(outer, parseState);
}

MobileStream::DataTransmissionState::DataTransmissionState(
		size_t numTransitions, Transition const * transitions) :
		CommandState(PSTR("AT+CIPSEND="), numTransitions, transitions) {
}

void MobileStream::DataTransmissionState::task(MobileStream* const outer,
		ParseState& parseState) const {
	int16_t prompt;
	int16_t numBytes;
	int16_t tmpNumBytes;
	int16_t i;

	// An interrupt can only have happened after waiting for the prompt. Return to there.
	if (parseState.progress == P_RETURNED_FROM_INTERRUPT) {
		parseState.progress = P_WAITING_TRANSMIT_WS;
	}

	// Are we done transmitting already?
	if (parseState.progress >= P_WAITING_INITIAL_WS) {
		DecisionState::task(outer, parseState);
		return;
	}

	// First send the command..
	if (parseState.progress == P_START) {
		parseState.responseTimeout = 0;
		// Make sure there is space for the command and a trailing CR.
		if ((size_t) outer->modem->txspace() >= strlen_P(_command)) {
			outer->modem->print_P(_command);
			parseState.progress = P_COMMAND_SENT;
		}
	}

	// Then send the message length..
	if (parseState.progress == P_COMMAND_SENT) {
		// Enough space for 4 digits and a CRLF.
		if (outer->modem->txspace() > 5) {
			outer->modem->printf_P(PSTR("%u"), parseState.numTxBytes);
			outer->modem->print_P(PSTR("\r\n"));
			parseState.progress = P_WAITING_TRANSMIT_WS;
		}
	}

	// Then wait for "> " from modem (skipping this step caused the modem to lose data!)
	// UNFORTUNATELY (cheap SIM900 crap!) URCs may come mixed with the > and we are chanceless.

	if (parseState.progress == P_WAITING_TRANSMIT_WS) {
		prompt = outer->modem->peek();
		if (parseState.responseTimeout++ >= 25) {
			// Lost the prompt, try again.
			parseState.responseTimeout = 0;
			parseState.beginState(parseState.state);
		}
		if (prompt == '\n' || prompt == '\r') {
			outer->modem->read();
		} else {
			if (prompt == '>' /* || prompt == ' '*/) {
				outer->modem->read(); // consume it.
				parseState.progress = P_TRANSMITTING;
			} else if (prompt >= 0) {
				// Receiving anything else at this stage means that an interrupt-like URC was received.
				parseState.progress = P_BUFFERING_RESPONSE_LINE;
				return; // now the decision state dung should handle it. The most recent input char was not consumed.
			}
		}
	}

	// Then send the data in the largest possible chunks.
	if (parseState.progress == P_TRANSMITTING) {
		numBytes = parseState.numTxBytes;

		tmpNumBytes = outer->userdata.backend_available();

		if (tmpNumBytes < numBytes)
			numBytes = tmpNumBytes;

		tmpNumBytes = outer->modem->txspace();

		if (tmpNumBytes < numBytes)
			numBytes = tmpNumBytes;

		for (i = 0; i < numBytes; i++) {
			outer->modem->write(outer->userdata.backend_read());
		}

		parseState.numTxBytes -= numBytes;
		if (parseState.numTxBytes == 0) {
			parseState.progress = P_WAITING_INITIAL_WS;
			parseState.bufptr = 0;
		}
	}
}

void MobileStream::DataReceptionState::task(MobileStream* const outer,
		ParseState& parseState) const {
	int numBytes;
	int tmpNumBytes;

	if (parseState.progress == P_START) {
		parseState.numRxBytes = 0;
		parseState.progress = P_RECEIVING_DATALENGTH;
	}

	// First, get the byte count terminated by ':' 
	if (parseState.progress == P_RECEIVING_DATALENGTH) {
		int data = outer->modem->read();
		if (data < 0)
			return;
		if (data == ':') {
			parseState.progress = P_RECEIVING_DATA;
		} else {
			parseState.numRxBytes *= 10;
			parseState.numRxBytes += (data - '0');
		}
	}

	// Then get the data in the largest possible chunks.
	if (parseState.progress == P_RECEIVING_DATA) {
		numBytes = parseState.numRxBytes;

		tmpNumBytes = outer->modem->available();

		if (tmpNumBytes < numBytes)
			numBytes = tmpNumBytes;

		tmpNumBytes = outer->userdata.backend_rxspace();

		if (tmpNumBytes < numBytes)
			numBytes = tmpNumBytes;

		if (numBytes < 0)
			numBytes = 0;

		size_t i;
		for (i = 0; i < numBytes; i++) {
			outer->userdata.backend_write(outer->modem->read());
		}
		parseState.numRxBytes -= numBytes;
		if (parseState.numRxBytes == 0) {
			parseState.restoreState();
		}
	}
}

const MobileStream::State* MobileStream::matchURCs(
		ParseState& parseState) const {
	uint8_t i;
	for (i = 0; i < numURCTransitions; i++) {
		if (parseState.match(URCTransitions[i].token)) {
			return URCTransitions[i].state;
		}
	}
	return NULL;
}

const MobileStream::State* MobileStream::matchInterrupts(
		ParseState& parseState) const {
	uint8_t i;
	for (i = 0; i < numURCInterrupts; i++) {
		if (parseState.match(URCInterrupts[i].token)) {
			return URCInterrupts[i].state;
		}
	}
	return NULL;
}

void MobileStream::begin(unsigned int rxSpace, unsigned int txSpace) {
	userdata.begin(rxSpace, txSpace);
	// The application is expected to init the serial port! Not we. However it might
	// make sense to do it anyway, reducing buffer sizes if possible.
	// With 57600 baud and 50 invocations/sec, there should still be enough space for
	// receiving 116 bytes though.
	// modem->begin(rxSpace, txSpace);
	parseState.beginState(parseState.savedState = &S_INITIAL);
}

/*
 * Begin message and end message events may be invoked from GCS_MAVLink!
 * Right now, they seem not necessary.
 */
void MobileStream::beginTransmitMessage(int16_t length) {
}
void MobileStream::endTransmitMessage(int16_t length) {
}

/*
 * Available incoming data.
 */
int MobileStream::available(void) {
	return userdata.available();
}

/*
 * Main driver of the state machine.
 */
void MobileStream::task(void) {
	do {
		parseState.state->task(this, parseState);
	} while (modem->available() > 0);
}
