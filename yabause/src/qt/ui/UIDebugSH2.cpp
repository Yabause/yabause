/*	Copyright 2012-2013 Theo Berkau <cwx@cyberwarriorx.com>

	This file is part of Yabause.

	Yabause is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Yabause is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Yabause; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "Settings.h"
#include "UIDebugSH2.h"
#include "../CommonDialogs.h"
#include "UIYabause.h"
#include "VolatileSettings.h"
#include <QProcess>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <filesystem>

int SH2Dis(SH2_struct *context, u32 addr, char *string)
{
   if(context->model == SHMT_SH1)
      SH2Disasm(addr, context->MappedMemoryReadWord(context, addr), 0, NULL, string);
   else
      SH2Disasm(addr, MappedMemoryReadWordNocache(context, addr), 0, NULL, string);
   return 2;
}

void SH2BreakpointHandler (SH2_struct *context, u32 addr, void *userdata)
{
   UIYabause* ui = QtYabause::mainWindow( false );

   if (context == SH1)
      emit ui->breakpointHandlerSH1(userdata == NULL ? true : false);
   else if (context == MSH2)
      emit ui->breakpointHandlerMSH2(userdata == NULL ? true : false);
   else
      emit ui->breakpointHandlerSSH2(userdata == NULL ? true : false);
}

UIDebugSH2::UIDebugSH2(UIDebugCPU::PROCTYPE proc, YabauseThread *mYabauseThread, QWidget* p )
	: UIDebugCPU( proc, mYabauseThread, p )
{
	switch (proc)
	{
		case UIDebugCPU::PROC_SH1:
			this->setWindowTitle(QtYabause::translate("Debug Master SH1"));
			gbRegisters->setTitle(QtYabause::translate("SH1 Registers"));
			debugSH2 = SH1;
			break;
		case UIDebugCPU::PROC_MSH2:
			this->setWindowTitle(QtYabause::translate("Debug Master SH2"));
			gbRegisters->setTitle(QtYabause::translate("SH2 Registers"));
			debugSH2 = MSH2;
			break;
		case UIDebugCPU::PROC_SSH2:
			this->setWindowTitle(QtYabause::translate("Debug Slave SH2"));
			gbRegisters->setTitle(QtYabause::translate("SH2 Registers"));
			debugSH2 = SSH2;
			break;
		default: break;
	}

	lwDisassembledCode->setContext(debugSH2);

   if (debugSH2)
   {
      const codebreakpoint_struct *cbp;
      const memorybreakpoint_struct *mbp;
      int i;

      cbp = SH2GetBreakpointList(debugSH2);
      mbp = SH2GetMemoryBreakpointList(debugSH2);

      for (i = 0; i < MAX_BREAKPOINTS; i++)
      {
         QString text;
         if (cbp[i].addr != 0xFFFFFFFF)
         {
            text.sprintf("%08X", (int)cbp[i].addr);
            lwCodeBreakpoints->addItem(text);
         }

         if (mbp[i].addr != 0xFFFFFFFF)
         {
            text.sprintf("%08X", (int)mbp[i].addr);
            lwMemoryBreakpoints->addItem(text);
         }
      }

      lwDisassembledCode->setDisassembleFunction((int (*)(void *, u32, char *))SH2Dis);
		if (debugSH2->model == SHMT_SH1)
			lwDisassembledCode->setEndAddress(0x09080000);
		else
			lwDisassembledCode->setEndAddress(0x06100000);
      lwDisassembledCode->setMinimumInstructionSize(2);
      gbBackTrace->setVisible( true );

      SH2SetBreakpointCallBack(debugSH2, (void (*)(void *, u32, void *))SH2BreakpointHandler, NULL);
   }

   updateAll();

   if (debugSH2 && debugSH2->trackInfLoop.enabled)
      pbReserved1->setText(QtYabause::translate("Loop Track Stop"));
   else
      pbReserved1->setText(QtYabause::translate("Loop Track Start"));
  pbReserved2->setText(QtYabause::translate("Loop Track Clear"));
	pbReserved3->setText(QtYabause::translate("Inline Assembly"));

  pbStepOver->setVisible( true );
  pbStepOut->setVisible( true );
  pbReserved1->setVisible( true );
  pbReserved2->setVisible( true );
	pbReserved3->setVisible( true );
  pbLoadCode->setVisible( true );
	connect( pbLoadCode, SIGNAL( clicked() ), this, SLOT( loadCodeAddress() ) );

  restoreAddr2line();
}
   
void UIDebugSH2::restoreAddr2line()
{
  Settings* settings = QtYabause::settings();
  addr2line = settings->value( "Debug/Addr2Line" ).toString();
}

void UIDebugSH2::updateRegList()
{
   int i;
   sh2regs_struct sh2regs;
   QString str;

   if (debugSH2 == NULL)
      return;

   SH2GetRegisters(debugSH2, &sh2regs);
   lwRegisters->clear();

   for (i = 0; i < 16; i++)
   {
      str.sprintf("R%02d =  %08X", i, (int)sh2regs.R[i]);
      lwRegisters->addItem(str);
   }

   // SR
   str.sprintf("SR =   %08X", (int)sh2regs.SR.all);
   lwRegisters->addItem(str);

   // GBR
   str.sprintf("GBR =  %08X", (int)sh2regs.GBR);
   lwRegisters->addItem(str);

   // VBR
   str.sprintf("VBR =  %08X", (int)sh2regs.VBR);
   lwRegisters->addItem(str);

   // MACH
   str.sprintf("MACH = %08X", (int)sh2regs.MACH);
   lwRegisters->addItem(str);

   // MACL
   str.sprintf("MACL = %08X", (int)sh2regs.MACL);
   lwRegisters->addItem(str);

   // PR
   str.sprintf("PR =   %08X", (int)sh2regs.PR);
   lwRegisters->addItem(str);

   // PC
   str.sprintf("PC =   %08X", (int)sh2regs.PC);
   lwRegisters->addItem(str);
}

void UIDebugSH2::updateCodeList(u32 addr)
{
   addr &= 0x0FFFFFFF;
   lwDisassembledCode->goToAddress(addr);
   lwDisassembledCode->setPC(addr);
}

void UIDebugSH2::updateBackTrace()
{
   int size;
   u32 *addr=SH2GetBacktraceList(debugSH2, &size);

   lwBackTrace->clear();
   for (int i = 0; i < size; i++)
      lwBackTrace->addItem(QString("%1").arg(addr[i], 8, 16, QChar('0')).toUpper());
   lwBackTrace->addItem(QString("%1").arg(debugSH2->regs.PC, 8, 16, QChar('0')).toUpper());
}

void UIDebugSH2::updateTrackInfLoop()
{
   if (debugSH2)
   {
      tilInfo_struct *match=debugSH2->trackInfLoop.match;

      twTrackInfLoop->clearContents();
      twTrackInfLoop->setRowCount(0);
      twTrackInfLoop->setSortingEnabled(false);
      for (int i = 0; i < debugSH2->trackInfLoop.num; i++)
      {
         twTrackInfLoop->insertRow(i);
         QTableWidgetItem *newItem = new QTableWidgetItem(QString("%1").arg(match[i].addr, 8, 16, QChar('0')).toUpper());
         twTrackInfLoop->setItem(i, 0, newItem);

         newItem = new QTableWidgetItem();
         newItem->setData(Qt::DisplayRole, (qulonglong) match[i].count);
         twTrackInfLoop->setItem(i, 1, newItem);
      }
      twTrackInfLoop->setSortingEnabled(true);
   }
}

void UIDebugSH2::loadCodeAddress()
{
  sh2regs_struct sh2regs;
  SH2GetRegisters(debugSH2, &sh2regs);

  std::stringstream currentAddress;
  currentAddress << std::hex << sh2regs.PC;

	bool ok = false;
  const std::string newAddress = QInputDialog::getText(this, tr("Input code address"),
		tr("Address (hex):"), QLineEdit::Normal,
    QString::fromStdString(currentAddress.str()), &ok).toStdString();
        
	if (ok)
    updateCodePage(static_cast<uint32_t>(std::stoull(newAddress, nullptr, 16)));
}
   
void UIDebugSH2::updateCodePage(u32 evaluateAddress)
{
  DebugPrintf(MainLog, __FILE__, __LINE__, "Address to inspect %x\n", evaluateAddress);
  if (addr2line.isEmpty())
    restoreAddr2line();

  const QString program{ addr2line };
  std::filesystem::path elfPath;
		
  VolatileSettings *vs = QtYabause::volatileSettings();
	if ( vs->value( "General/CdRom" ) != CDCORE_ISO )
  {
    DebugPrintf(MainLog, __FILE__, __LINE__, "Not using ISO, ignoring code\n");
    return;
  }
  else
  {
    const QString isoPathString{ vs->value( "General/CdRomISO" ).toString() };
    const std::filesystem::path isoPath{ isoPathString.toStdString() };

    // Search first at the './build/' directory
    const std::filesystem::path folder{ isoPath.parent_path() };
    const std::string elfFilename{ isoPath.filename().stem().string() + ".elf" };
    const std::filesystem::path atBuildFolder{ folder / "build" / elfFilename };
    const std::filesystem::path atLocalFolder{ folder / elfFilename };
    
    if ( std::filesystem::exists( atBuildFolder ) )
      elfPath = atBuildFolder;
    else if ( std::filesystem::exists( atLocalFolder ) )
      elfPath = atLocalFolder;
  }

  if ( elfPath.empty() )
  {
    DebugPrintf(MainLog, __FILE__, __LINE__, "Could not find elf file, ignoring code\n");
    return;
  }

  std::stringstream hexAddress;
  hexAddress << std::setfill('0') << std::setw(8) << std::hex
             << evaluateAddress;

  QStringList arguments;
  arguments.push_back("-a");
  arguments.push_back(QString::fromStdString(hexAddress.str()));
  arguments.push_back("-p");
  arguments.push_back("-i");
  arguments.push_back("-f");
  arguments.push_back("-C");
  arguments.push_back("-e");
  arguments.push_back(QString::fromStdString(elfPath.string()));

  QProcess addr2lineProgram;
  addr2lineProgram.start(program, arguments);
  addr2lineProgram.waitForFinished();

  std::string commandString;
  commandString += program.toStdString() + " " + arguments.join(" ").toStdString();

  const QByteArray pstdout = addr2lineProgram.readAllStandardOutput();
  if (addr2lineProgram.exitCode() != 0) {
    const QByteArray pstderr = addr2lineProgram.readAllStandardError();
    const std::string outputString =
        pstdout.toStdString() + " / " + pstderr.toStdString();

    DebugPrintf(MainLog, __FILE__, __LINE__, "Cmd: %s\n",
                commandString.c_str());
    DebugPrintf(MainLog, __FILE__, __LINE__, "Output (%d): %s\n",
                addr2lineProgram.exitCode(), outputString.c_str());
  
    codeBrowser->setText(QString::fromStdString(outputString));
  } else {

    // QRegularExpression re("(0x[0-9a-fA-F]+):\\s*(.+?(?= at )) at (.+?(?=:[0-9]+)):([0-9]+)");
    QRegularExpression re("(0x[0-9a-fA-F]+):\\s*(.+?(?= at )) at (.+?(?=:[0-9]+)):([0-9]+)"
      "(\\s*\\(inlined by\\)\\s*(.+?(?= at )) at (.+?(?=:[0-9]+)):([0-9]+))?");

    QRegularExpressionMatch match = re.match(pstdout);
    bool hasMatch = match.hasMatch();
    
    if (match.hasMatch())
    {
      const int filenameGroup = match.lastCapturedIndex() == 8 ? 7 : 3;
      const int lineGroup = match.lastCapturedIndex() == 8 ? 8 : 4;

      const std::filesystem::path filename{match.captured(filenameGroup).toStdString()};
      const int line{ std::stoi(match.captured(lineGroup).toStdString()) };

      std::ifstream inputFile(filename, std::ios_base::in);
      inputFile.seekg(0, std::ios_base::end);
      const size_t fileSize = inputFile.tellg();
      inputFile.seekg(0, std::ios_base::beg);

      std::string tmpString(fileSize, '\0');
      inputFile.read(&tmpString[0], fileSize);

      const std::string tooltip{ filename.string() + ":" + std::to_string(line) };
      
      codeTab->setTabText(1, QString::fromStdString(filename.filename().string()));
      codeTab->setTabToolTip(1, QString::fromStdString(tooltip));
      codeBrowser->setText(QString::fromStdString(tmpString));

      QTextBlock lineBlock = codeBrowser->document()->findBlockByLineNumber(line - 1);
      codeBrowser->moveCursor(QTextCursor::End);
      codeBrowser->setTextCursor(QTextCursor(lineBlock));

      QTextBlockFormat highlightFormat;
      highlightFormat.setBackground(Qt::yellow);
      highlightFormat.setNonBreakableLines(true);
      highlightFormat.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysBefore);

      QTextCursor newCursor(codeBrowser->textCursor());
      newCursor.setPosition(lineBlock.position());
      newCursor.select(QTextCursor::LineUnderCursor);
      newCursor.setBlockFormat(highlightFormat);
    }
    else
    {
      codeTab->setTabText(1, "Source");
      if (addr2line.isEmpty())
        codeBrowser->setText("addr2line utility is not configured properly, source not available.");
      else
        codeBrowser->setText(QString("Source not found or available. Stdout was:\n") + pstdout);
    }
  }
}

void UIDebugSH2::updateAll()
{
   updateRegList();
   if (debugSH2)
   {
      sh2regs_struct sh2regs;

      SH2GetRegisters(debugSH2, &sh2regs);
      updateCodeList(sh2regs.PC);
      updateBackTrace();
      updateTrackInfLoop();
      updateCodePage(sh2regs.PC);
   }
}

u32 UIDebugSH2::getRegister(int index, int *size)
{
   sh2regs_struct sh2regs;
   u32 value;

   SH2GetRegisters(debugSH2, &sh2regs);

   if (index < 16)
      value = sh2regs.R[index];
   else 
   {
      switch(index)
      {
         case 16:
            value = sh2regs.SR.all;
            break;
         case 17:
            value = sh2regs.GBR;
            break;
         case 18:
            value = sh2regs.VBR;
            break;
         case 19:
            value = sh2regs.MACH;
            break;
         case 20:
            value = sh2regs.MACL;
            break;
         case 21:
            value = sh2regs.PR;
            break;
         case 22:
            value = sh2regs.PC;
            break;
			default:
				value = 0;
				break;
      }
   }

   *size = 4;
   return value;
}

void UIDebugSH2::setRegister(int index, u32 value)
{
   sh2regs_struct sh2regs;

   SH2GetRegisters(debugSH2, &sh2regs);

   if (index < 16)
      sh2regs.R[index] = value;
   else
   {
      switch(index)
      {
         case 16:
            sh2regs.SR.all = value;
            break;
         case 17:
            sh2regs.GBR = value;
            break;
         case 18:
            sh2regs.VBR = value;
            break;
         case 19:
            sh2regs.MACH = value;
            break;
         case 20:
            sh2regs.MACL = value;
            break;
         case 21:
            sh2regs.PR = value;
            break;
         case 22:
            sh2regs.PC = value;
            updateCodeList(sh2regs.PC);
            break;
      }
   }

   SH2SetRegisters(debugSH2, &sh2regs);
}

bool UIDebugSH2::addCodeBreakpoint(u32 addr)
{
	if (!debugSH2)
		return false;
   return SH2AddCodeBreakpoint(debugSH2, addr) == 0;     
}

bool UIDebugSH2::delCodeBreakpoint(u32 addr)
{
	if (!debugSH2)
		return false;
    return SH2DelCodeBreakpoint(debugSH2, addr) == 0;
}

bool UIDebugSH2::addMemoryBreakpoint(u32 addr, u32 flags)
{
	if (!debugSH2)
		return false;
   return SH2AddMemoryBreakpoint(debugSH2, addr, flags) == 0;     
}

bool UIDebugSH2::delMemoryBreakpoint(u32 addr)
{
	if (!debugSH2)
		return false;
    return SH2DelMemoryBreakpoint(debugSH2, addr) == 0;
}

void UIDebugSH2::stepInto()
{
   if (debugSH2)
   {
      SH2Step(debugSH2);
      updateAll();
   }
}

void UIDebugSH2::stepOver()
{
   if (debugSH2)
   {
      if (SH2StepOver(debugSH2, (void (*)(void *, u32, void *))SH2BreakpointHandler) == 0)
         updateAll();
      else
         // Close dialog and wait
         this->accept(); 
   }

}

void UIDebugSH2::stepOut()
{
   if (debugSH2)
   {
      SH2StepOut(debugSH2, (void (*)(void *, u32, void *))SH2BreakpointHandler);

      // Close dialog and wait
      this->accept(); 
   }

}

void UIDebugSH2::reserved1()
{
   if (debugSH2)
   {
      if (!debugSH2->trackInfLoop.enabled)
      {
         SH2TrackInfLoopStart(debugSH2);
         pbReserved1->setText(QtYabause::translate("Loop Track Stop"));
      }
      else
      {
         SH2TrackInfLoopStop(debugSH2);
         pbReserved1->setText(QtYabause::translate("Loop Track Start"));
      }
   }
}

void UIDebugSH2::reserved2()
{
   if (debugSH2)
      SH2TrackInfLoopClear(debugSH2);
   updateAll();
}

void UIDebugSH2::reserved3()
{
	if (debugSH2)
	{
		bool ok;

		for(;;)
		{
			QString text = QInputDialog::getText(this, QtYabause::translate("Assembly code"), 
				QtYabause::translate("Enter new assembly code") + ":", QLineEdit::Normal,
				QString(), &ok);

			if (ok && !text.isEmpty())
			{			
				char errorMsg[512];
				int op = sh2iasm(text.toLatin1().data(), errorMsg);
				if (op != 0)
				{
					MappedMemoryWriteWordNocache(debugSH2, debugSH2->regs.PC, op);
					break;
				}
				else
					QMessageBox::critical(QApplication::activeWindow(), QtYabause::translate("Error"), QString(errorMsg));
			}
			else if (!ok)
				break;
		}
	}
	updateAll();
}

