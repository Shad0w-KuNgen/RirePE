#include"../RirePE/MainGUI.h"
std::vector<PacketData> packet_data_out;
std::vector<PacketData> packet_data_in;

void ClearAll() {
	packet_data_out.clear();
	packet_data_in.clear();
}

std::vector<PacketData>& GetOutPacketFormat() {
	return packet_data_out;
}

std::vector<PacketData>& GetInPacketFormat() {
	return packet_data_in;
}

bool AddFormat(PacketData &pd, PacketEditorMessage &pem) {
	// �p�P�b�g���b�N�ς�
	if (pd.lock) {
		return false;
	}
	// �p�P�b�g��o�^
	if (pem.header == SENDPACKET || pem.header == RECVPACKET) {
		pd.packet.resize(pem.Binary.length);
		memcpy_s(&pd.packet[0], pem.Binary.length, pem.Binary.packet, pem.Binary.length);
		pd.addr = pem.addr;

		// Send�̏ꍇ�͐�ɑS��Encode����ォ��p�P�b�g�̃T�C�Y����������
		if (pd.packet.size() && pd.packet.size() == pd.used) {
			// �S�Ẵf�[�^�����p���ꂽ
			if (pd.status == 0) {
				pd.status = 1;
			}
		}

		// �p�P�b�g���b�N
		if (pem.header == SENDPACKET) {
			pd.lock = TRUE;

			// �����ɓ�f�[�^������ꍇ
			if (pd.used < pd.packet.size()) {
				PacketFormat unk;
				unk.type = WHEREFROM;
				unk.pos = pd.used;
				unk.size = pd.packet.size() - pd.used;
				unk.addr = 0;
				pd.format.push_back(unk);
				pd.status = -1;
			}
			else {
				pd.status = 1;
			}

			// ���O�C���p�P�b�g�̃p�X���[�h������
			//RemovePassword(pd);
		}
		return true;
	}

	// �p�P�b�g���b�N
	if (pem.header == DECODEEND) {
		pd.lock = TRUE;
		if (pd.used < pd.packet.size()) {
			PacketFormat unk;
			unk.type = NOTUSED;
			unk.pos = pd.used;
			unk.size = pd.packet.size() - pd.used;
			unk.addr = 0;
			pd.format.push_back(unk);
			pd.status = -1;
		}
		return true;
	}

	// �����decode or encode�o���Ă��Ȃ��ꍇ�͌����߂���
	if (pd.used < pem.Extra.pos) {
		PacketFormat unk;
		unk.type = UNKNOWNDATA;
		unk.pos = pd.used;
		unk.size = pem.Extra.pos - pd.used;
		unk.addr = 0;
		pd.format.push_back(unk);
		pd.status = -1;
		pd.used += unk.size;
		return false;
	}

	// �t�H�[�}�b�g��o�^
	PacketFormat pf;
	pf.type = pem.header;
	pf.pos = pem.Extra.pos;
	pf.size = pem.Extra.size;
	pf.addr = pem.addr;
	pd.format.push_back(pf);

	// ��Ԃ�ύX
	pd.used += pf.size;
	// Recv�̏ꍇ�͐�Ƀp�P�b�g�̃T�C�Y���������Ă���
	if (pd.packet.size() && pd.packet.size() == pd.used) {
		// �S�Ẵf�[�^�����p���ꂽ
		if (pd.status == 0) {
			pd.status = 1;
		}
	}
	return true;
}

bool AddRecvPacket(PacketEditorMessage &pem) {
	for (size_t i = 0; i < packet_data_in.size(); i++) {
		if (packet_data_in[i].id == pem.id) {
			AddFormat(packet_data_in[i], pem);
			return false;
		}
	}

	PacketData pd;
	pd.id = pem.id;
	pd.type = RECVPACKET;
	pd.status = 0;
	pd.used = 0;
	pd.lock = FALSE;
	AddFormat(pd, pem);
	packet_data_in.push_back(pd);
	return true;
}

bool AddSendPacket(PacketEditorMessage &pem) {
	for (size_t i = 0; i < packet_data_out.size(); i++) {
		if (packet_data_out[i].id == pem.id) {
			AddFormat(packet_data_out[i], pem);
			return false;
		}
	}

	PacketData pd;
	pd.id = pem.id;
	pd.type = SENDPACKET;
	pd.status = 0;
	pd.used = 0;
	pd.lock = FALSE;
	AddFormat(pd, pem);
	packet_data_out.push_back(pd);
	return true;
}
WORD HeaderStringToWORD(std::wstring wHeaderText) {
	WORD wHeader = 0xFFFF;
	swscanf_s(wHeaderText.c_str(), L"@%hX", &wHeader);
	return wHeader;
}

// �N���C�A���g����̃p�P�b�g�̏���
bool LoggerCommunicate(PipeServerThread& psh) {
	SetInfo(L"Connected");

	std::vector<BYTE> data;
	bool bBlock = false;
	while (psh.Recv(data)) {
		PacketEditorMessage &pem = (PacketEditorMessage&)data[0];

		/*
		if (pem.header == SENDPACKET && SearchHeaders(vBlockSendHeaders, *(WORD *)&pem.Binary.packet[0])) {
			psh.Send(L"Block");
			bBlock = true;
		}
		else if (pem.header == RECVPACKET && SearchHeaders(vBlockRecvHeaders, *(WORD *)&pem.Binary.packet[0])) {
			psh.Send(L"Block");
			bBlock = true;
		}
		else {
			if (pem.header == SENDPACKET || pem.header == RECVPACKET) {
				psh.Send(L"OK");
			}
			bBlock = false;
		}
		*/
		if (pem.header == SENDPACKET || pem.header == RECVPACKET) {
			psh.Send(L"OK");
		}
		bBlock = false;

		if (pem.header == SENDPACKET) {
			UpdateLogger(pem, bBlock);
			AddSendPacket(pem);
			continue;
		}

		if (pem.header == RECVPACKET) {
			UpdateLogger(pem, bBlock);
			AddRecvPacket(pem);
			continue;
		}

		if (ENCODEHEADER <= pem.header && pem.header <= ENCODEBUFFER) {
			AddSendPacket(pem);
			continue;
		}

		if (DECODEHEADER <= pem.header && pem.header <= DECODEBUFFER) {
			AddRecvPacket(pem);
			continue;
		}

		if (pem.header == DECODEEND) {
			AddRecvPacket(pem);
			continue;
		}
	}
	SetInfo(L"Disconnected");
	return true;
}

bool RunPacketLoggerPipe() {
	PipeServer ps(PE_LOGGER_PIPE_NAME);
	ps.SetCommunicate(LoggerCommunicate);
	return ps.Run();
}

bool PacketLogger() {
	HANDLE hThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)RunPacketLoggerPipe, NULL, NULL, NULL);
	if (!hThread) {
		return false;
	}
	CloseHandle(hThread);
	return true;
}