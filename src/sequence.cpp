#pragma comment(user, "license")

#include "modernmidi.h"
#include "sequence.h"
#include "midi_message.h"
#include <algorithm>

namespace mm 
{
	// Read a MIDI-style variable-length integer (big-endian value in groups of 7 bits,
	// with top bit set to signify that another byte follows). 
	uint32_t read_variable_length(uint8_t const *& data)
	{
		uint32_t result = 0;
		while (true) 
		{
			uint8_t b = *data++;
			if (b & 0x80) 
			{
				result += (b & 0x7F);
				result <<= 7;
			} 
			else 
			{
				return result + b; // b is the last byte
			}
		}
	}
	
	void read_bytes(std::vector<unsigned char> & buffer, uint8_t const *& data, int num)
	{
		for (int i = 0; i < num; ++i)
		{
			buffer.push_back(uint8_t(*data++));
		}
	}

	uint16_t read_uint16_be(uint8_t const *& data)
	{
		uint16_t result = int(*data++) << 8;
		result += int(*data++);
		return result;
	}
	
	uint32_t read_uint24_be(uint8_t const *& data)
	{
		uint32_t result = int(*data++) << 16;
		result += int(*data++) << 8;
		result += int(*data++);
		return result;
	}
	
	uint32_t read_uint32_be(uint8_t const *& data)
	{
		uint32_t result = int(*data++) << 24;
		result += int(*data++) << 16;
		result += int(*data++) << 8;
		result += int(*data++);
		return result;
	}
	
	// Parse Validators
    // [ ] Bad file name
    // [ ] bad header
    // [ ] unknown header type
    // [ ] bad header size
    // [ ] bad type
    // [ ] bad tmecode
    // [ ] header too short
    // [ ] track too short
    // [ ] event too short
    
	TrackEvent * parseEvent(int tick, int track, uint8_t const *& dataStart, MessageType lastEventTypeByte)
	{
		MessageType type = (MessageType) *dataStart++;
		
		auto m = std::make_shared<MidiMessage>();
		TrackEvent * event = new TrackEvent(tick, track, m);

		if (((uint8_t) type & 0xF) == 0xF) 
		{
			// Meta event 
			if ((uint8_t) type == 0xFF) 
			{
				MetaEventType subtype = (MetaEventType) *dataStart++;
				int length = read_variable_length(dataStart); 

				event->m->data = std::vector<unsigned char>(std::max(length, 3), 0);
				event->m->data[0] = (uint8_t) type;
				event->m->data[1] = (uint8_t) subtype;
				event->m->data[2] = length;

				switch(subtype) 
				{
					case MetaEventType::SEQUENCE_NUMBER: 
					{
						if (length != 2) throw std::runtime_error("Expected length for sequenceNumber event is 2");
						read_bytes(event->m->data, dataStart, 2);
						return event;
					}
					case MetaEventType::TEXT: 
					case MetaEventType::COPYRIGHT: 
					case MetaEventType::TRACK_NAME: 
					case MetaEventType::INSTRUMENT: 
					case MetaEventType::LYRIC: 
					case MetaEventType::MARKER: 
					case MetaEventType::CUE: 						
					{
						read_bytes(event->m->data, dataStart, length);
						return event;
					}
					
					case MetaEventType::END_OF_TRACK: 
					{
						if (length != 0) throw std::runtime_error("Expected length for endOfTrack event is 0");
						return event;
					}
					case MetaEventType::TEMPO_CHANGE: 
					{
						if (length != 3) throw std::runtime_error("Expected length for setTempo event is 3");
						event->m->data[3] = read_uint24_be(dataStart);
						return event;
					}
					case MetaEventType::SMPTE_OFFSET: 
					{
						if (length != 5) throw std::runtime_error("Expected length for smpteOffset event is 5");
						read_bytes(event->m->data, dataStart, length);
						return event;
					}
					case MetaEventType::TIME_SIGNATURE: 
					{
						if (length != 4) throw std::runtime_error("Expected length for timeSignature event is 4");
						read_bytes(event->m->data, dataStart, length);
						return event;
					}
					case MetaEventType::KEY_SIGNATURE: 
					{
						if (length != 2) throw std::runtime_error("Expected length for keySignature event is 2");
						read_bytes(event->m->data, dataStart, length);
						return event;
					}
					case MetaEventType::PROPRIETARY: 
					{
						read_bytes(event->m->data, dataStart, length);
						return event;
					}
				}

				// Unknown events? 
				read_bytes(event->m->data, dataStart, length);
				return event;
			}

			else if (type == MessageType::SYSTEM_EXCLUSIVE) 
			{
				int length = read_variable_length(dataStart);
				read_bytes(event->m->data, dataStart, length);
				return event;
			}

			else if (type == MessageType::EOX)
			{
				int length = read_variable_length(dataStart);
				read_bytes(event->m->data, dataStart, length);
				return event;
			}
			else 
			{
				throw std::runtime_error("Unrecognised MIDI event type byte");
			}
		}

		// Channel events
		else 
		{
			event->m->data = std::vector<unsigned char>(3, 0);
			event->m->data[0] = (uint8_t) type;

			// Running status... 
			if (((uint8_t) type & 0x80) == 0) 
			{
				// Reuse lastEventTypeByte as the event type.
				// eventTypeByte is actually the first parameter
				event->m->data[0] = (uint8_t) type;
				type = lastEventTypeByte;
			}
			else 
			{
				event->m->data[1] = uint8_t(*dataStart++);
				lastEventTypeByte = type;
			}

			// Just in case
			event->m->data[2] = 0xFF;

			switch (MessageType((uint8_t) type & 0xF0))
			{
				case MessageType::NOTE_OFF:
					event->m->data[2] = uint8_t(*dataStart++);
					return event;
				case MessageType::NOTE_ON:
					event->m->data[2] = uint8_t(*dataStart++);
					return event;
				case MessageType::POLY_PRESSURE:
					event->m->data[2] = uint8_t(*dataStart++);
					return event;
				case MessageType::CONTROL_CHANGE:
					event->m->data[2] = uint8_t(*dataStart++);
					return event;
				case MessageType::PROGRAM_CHANGE:
					return event;
				case MessageType::AFTERTOUCH:
					return event;
				case MessageType::PITCH_BEND: 
					event->m->data[2] = uint8_t(*dataStart++);
					return event;
				default:
					throw std::runtime_error("Unrecognised MIDI event type");
			}
		}
		throw std::runtime_error("Unparsed event");
	}

	MidiSequence::MidiSequence() : tracks(0), ticksPerBeat(240), startingTempo(120)
	{
		
	}

	MidiSequence::~MidiSequence()
	{
		clearTracks();
	}
	
	void MidiSequence::clearTracks()
	{
		tracks.clear();
	}
	
	void MidiSequence::parseInternal(const std::vector<uint8_t> & buffer)
	{
		const uint8_t * dataPtr = buffer.data();
		
		int headerId = read_uint32_be(dataPtr);
		int headerLength = read_uint32_be(dataPtr);

		if (headerId != 'MThd' || headerLength != 6) 
		{
			std::cerr << "Bad .mid file - couldn't parse header" << std::endl;
			return;
		}
		
		read_uint16_be(dataPtr); //@tofix format type -> save for later eventually 

		int trackCount = read_uint16_be(dataPtr);
		int timeDivision = read_uint16_be(dataPtr);
		
		// CBB: deal with the SMPTE style time coding
		// timeDivision is described here http://www.sonicspot.com/guide/midifiles.html
		if (timeDivision & 0x8000) 
		{
			std::cerr << "Found SMPTE time frames" << std::endl;
			//int fps = (timeDivision >> 16) & 0x7f;
			//int ticksPerFrame = timeDivision & 0xff;
			// given beats per second, timeDivision should be derivable.
			return;
		}
		
		startingTempo = 120.0f; // midi default 
		ticksPerBeat = float(timeDivision); // ticks per beat (a beat is defined as a quarter note)
		
		for (int i = 0; i < trackCount; ++i) 
		{
			MidiTrack track;

			headerId = read_uint32_be(dataPtr);
			headerLength = read_uint32_be(dataPtr);

			if (headerId != 'MTrk') 
			{
				throw std::runtime_error("Bad .mid file - couldn't find track header");
			}

			uint8_t const * dataEnd = dataPtr + headerLength;

			MessageType runningEvent = MessageType::NOT_CHANNEL;

			while (dataPtr < dataEnd) 
			{
				auto tick = read_variable_length(dataPtr);
				auto ev = std::shared_ptr<TrackEvent>(parseEvent(tick, i, dataPtr, runningEvent));

				if (ev->m->isMetaEvent() == false)
				{
					runningEvent = MessageType(ev->m->data[0]);
				}
				
				track.push_back(ev);
			}

			tracks.push_back(track);
		}

	}

	// In ticks
	double MidiSequence::getEndTime()
	{
		double totalLength = 0;
		for (const auto t : tracks)
		{
			double localLength = 0;
			for (const auto e : t)
			{
				localLength += e->tick;
			}

			if (localLength > totalLength) 
			{
				totalLength = localLength;
			}
		}
		return totalLength;
	}

	void MidiSequence::parse(const std::vector<uint8_t> & buffer)
	{
		clearTracks();
		parseInternal(buffer);
	}

} // mm
