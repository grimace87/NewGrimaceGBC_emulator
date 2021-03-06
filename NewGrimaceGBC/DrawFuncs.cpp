
#include "Class.h"

// Used for background and window:
GLuint ColourPaletteTrans1[4] = {0xffffffff, 0x90b0f0ff, 0x1040a0ff, 0x000000ff};
// Used for one sprite palette:
GLuint ColourPaletteTrans2[4] = {0xffffffff, 0xf05050ff, 0xa02020ff, 0x000000ff};
// Used for the other sprite palette:
GLuint ColourPaletteTrans3[4] = {0xffffffff, 0xa0a0a0ff, 0x404040ff, 0x000000ff};

// Translated palettes containing RGBA colours
GLuint TranslatedPaletteBG[4];
GLuint TranslatedPaletteObj[8];

// Palette translation indices for fetching SGB colours
unsigned int SGB_TranslatePaletteBG[4];
unsigned int SGB_TranslatePaletteObj[8];

void GGBC::TranslatePaletteBG(unsigned int PalData) {
    TranslatedPaletteBG[0] = ColourPaletteTrans1[PalData & 0x03];
    TranslatedPaletteBG[1] = ColourPaletteTrans1[(PalData & 0x0c)/4];
    TranslatedPaletteBG[2] = ColourPaletteTrans1[(PalData & 0x30)/16];
    TranslatedPaletteBG[3] = ColourPaletteTrans1[(PalData & 0xc0)/64];
	SGB_TranslatePaletteBG[0] = PalData & 0x03;
	SGB_TranslatePaletteBG[1] = (PalData & 0x0c) / 4;
	SGB_TranslatePaletteBG[2] = (PalData & 0x30) / 16;
	SGB_TranslatePaletteBG[3] = (PalData & 0xc0) / 64;
}

void GGBC::TranslatePaletteObj1(unsigned int PalData) {
    TranslatedPaletteObj[1] = ColourPaletteTrans2[(PalData & 0x0c)/4];
    TranslatedPaletteObj[2] = ColourPaletteTrans2[(PalData & 0x30)/16];
    TranslatedPaletteObj[3] = ColourPaletteTrans2[(PalData & 0xc0)/64];
	SGB_TranslatePaletteObj[1] = (PalData & 0x0c) / 4;
	SGB_TranslatePaletteObj[2] = (PalData & 0x30) / 16;
	SGB_TranslatePaletteObj[3] = (PalData & 0xc0) / 64;
}

void GGBC::TranslatePaletteObj2(unsigned int PalData) {
    TranslatedPaletteObj[5] = ColourPaletteTrans3[(PalData & 0x0c)/4];
    TranslatedPaletteObj[6] = ColourPaletteTrans3[(PalData & 0x30)/16];
    TranslatedPaletteObj[7] = ColourPaletteTrans3[(PalData & 0xc0)/64];
	SGB_TranslatePaletteObj[5] = (PalData & 0x0c) / 4;
	SGB_TranslatePaletteObj[6] = (PalData & 0x30) / 16;
	SGB_TranslatePaletteObj[7] = (PalData & 0xc0) / 64;
}

void GGBC::ReadLine_GB() {
    
    // Get relevant parameters from status registers and such:
    const unsigned char LCDCtrl = IOPorts[0x40];
    const unsigned char LCDStatus = IOPorts[0x41];
    unsigned char ScrY = IOPorts[0x42];
    unsigned char ScrX = IOPorts[0x43];
    const unsigned int LineNo = IOPorts[0x44];

    // More variables
    unsigned int Offset, Max;
    unsigned int TileSetBase;
    unsigned int TileMapBase;
    unsigned int PixX, PixY, TileX, TileY;
    unsigned int TileNo;
    GLuint* DstPointer;
    unsigned int* TilesetPointer;

    // Sprite-specific stuff:
    unsigned int Flags;
    unsigned int PaletteOffset;
    unsigned int PixInc;
    unsigned int GetPix;
    
    // Check if LCD is disabled or all elements (BG, window, sprites) are disabled (write a black row if that's the case):
    if ((LCDCtrl & 0x80) == 0x00 || (LCDCtrl & 0x23) == 0x00) {
        DstPointer = &ImgData[LineNo*160];
        for (PixX = 0; PixX < 160; PixX++)
            *DstPointer++ = 0x000000ff;
        return;
    }

	TileSetBase = LCDCtrl & 0x10 ? 0x0000 : 0x0100;
	TileMapBase = LCDCtrl & 0x08 ? 0x1c00 : 0x1800;

    // Check for BG enabled first:
    if (LCDCtrl & 0x01) {
        
        // Set point to draw to
        DstPointer = &ImgData[160*LineNo];

        // Set starting point of VRAM data to read, in terms of pixel coordinates within the tile, and tile coordinates within the tilemap
        PixX = ScrX % 8;
        PixY = (LineNo + ScrY) % 8;
        TileX = ScrX / 8;
        TileY = ((LineNo + ScrY) % 256) / 8;

        // Draw first 20 tiles (including partial leftmost tile)
        for (Offset = 0; Offset < 20; Offset++) {

            // Get tile no. and point to the tileset data to read
            TileNo = VRAM[TileMapBase + 32*TileY + TileX];
            if (TileNo < 127) TileNo += TileSetBase;
            TilesetPointer = &TileSet[TileNo*64 + 8*PixY + PixX];
            
            // Draw up to 8 pixels of this tile
            while (PixX < 8) {
                *DstPointer++ = TranslatedPaletteBG[*TilesetPointer++];
                PixX++;
            }
            
            // Reset pixel counter and get next tile coordinates
            PixX = 0;
            TileX = (TileX + 1) % 32;

        }
        
        // Draw partial 21st tile. Find out how many pixels of it to draw.
        Max = ScrX % 8;

        // Get tile no
        TileNo = VRAM[TileMapBase + 32*TileY + TileX];
        if (TileNo < 127) TileNo += TileSetBase;
        TilesetPointer = &TileSet[TileNo*64 + 8*PixY + PixX];
        
        // Draw up to 8 pixels of this tile
        while (PixX < Max) {
            *DstPointer++ = TranslatedPaletteBG[*TilesetPointer++];
            PixX++;
        }
        
    }
    
    // Check for window enabled first, and check for window onscreen:
    ScrX = IOPorts[0x4b];
    ScrY = IOPorts[0x4a];
    TileMapBase = LCDCtrl & 0x40 ? 0x1c00 : 0x1800;
    if (((LCDCtrl & 0x20) != 0x00) && (ScrX < 167) && (ScrY <= LineNo)) {
        
        // Subtract 7 from window X pos
        if (ScrX > 6) ScrX -= 7;

        // Set point to draw to
        DstPointer = &ImgData[160*LineNo + ScrX];

		// Set starting point of VRAM data to read, in terms of pixel coordinates within the tile, and tile coordinates within the tilemap
        PixX = 0;
        PixY = (LineNo - ScrY) % 8;
        TileX = 0;
        TileY = (LineNo - ScrY) / 8;

        // Check how many complete tiles to draw
        Max = (160 - ScrX) / 8;

        // Draw all the complete tiles until the end of the line
        for (Offset = 0; Offset < Max; Offset++) {

            // Get tile no
            TileNo = VRAM[TileMapBase + 32*TileY + TileX];
            if (TileNo < 127) TileNo += TileSetBase;
            TilesetPointer = &TileSet[TileNo*64 + 8*PixY + PixX];
            
            // Draw the 8 pixels of this tile
            while (PixX < 8) {
                *DstPointer++ = TranslatedPaletteBG[*TilesetPointer++];
                PixX++;
            }
            
            // Reset pixel counter and get next tile coordinates
            PixX = 0;
            TileX = (TileX + 1) % 32;

        }
        
        // Draw partial last tile. Find out how many pixels of it to draw.
        Max = ScrX % 8;

        // Get tile no
        TileNo = VRAM[TileMapBase + 32*TileY + TileX];
        if (TileNo < 127) TileNo += TileSetBase;
        TilesetPointer = &TileSet[TileNo*64 + 8*PixY + PixX];
        
        // Draw up to 8 pixels of this tile
        while (PixX < Offset) {
            *DstPointer++ = TranslatedPaletteBG[*TilesetPointer++];
            PixX++;
        }
        
    }
    
    // Check for sprites enabled first:
    if (LCDCtrl & 0x02) {
        
		const unsigned char LargeSprites = LCDCtrl & 0x04;

        // Draw each sprite that is visible
        for (Offset = 156; Offset < 160; Offset -= 4) {
			
            // Get coords check if off-screen
            ScrY = OAM[Offset];
			if (ScrY == 0) continue;
            if (ScrY > 159) continue;
            ScrX = OAM[Offset+1];
			if (ScrX == 0) continue;
            if (ScrX > 167) continue;

			// Check if the current scan line would pass through an 8x16 sprite
            if (LineNo + 16 < ScrY) continue;
            if (LineNo >= ScrY) continue;
            TileNo = OAM[Offset+2];
            Flags = OAM[Offset+3];
            
            // Check sprite size, and accordingly re-assess visibility and adjust tilemap index
            if (LargeSprites) {
                if (LineNo + 8 >= ScrY) TileNo |= 0x01;
                else TileNo &= 0xfe;
                
            }
            else if (LineNo + 8 >= ScrY) continue;

			// Get pixel row within tile that will be drawn
            PixY = (LineNo + 16 - ScrY) % 8;
            
			// Set which palette to draw with
            PaletteOffset = Flags & 0x10 ? 4 : 0;
			GLuint ColourZero = TranslatedPaletteBG[0];
            
            // Get first pixel in tile row to draw, plus how many pixels to draw, and adjust the starting point to write to in the image
            if (ScrX < 8) {
                PixX = 8 - ScrX;
                Max = ScrX;
                ScrX = 0;
            }
            else if (ScrX > 160) {
                PixX = 0;
                Max = 168 - ScrX;
                ScrX -= 8;
            }
            else {
                PixX = 0;
                Max = 8;
                ScrX -= 8;
            }

			// Adjust X if horizontally flipping
			int TilesetPointerDirection;
			if (Flags & 0x20) {
				PixX = 7 - PixX;
				TilesetPointerDirection = -1;
			}
			else TilesetPointerDirection = 1;

			// Adjust Y if vertically flipping
			if (Flags & 0x40)
				PixY = 7 - PixY;

			// Get priority flag
			unsigned int BackgroundPriority = Flags & 0x80;

            // Set point to draw to
            DstPointer = &ImgData[160*LineNo + ScrX];

            // Get pointer to tile data
            TilesetPointer = &TileSet[TileNo*64 + 8*PixY + PixX];
            
            // Draw up to 8 pixels of this tile (skipping over transparent pixels with palette index 0, or if obscured by the background)
			PixX = 0;
            while (PixX < Max) {
                GetPix = *TilesetPointer;
				TilesetPointer += TilesetPointerDirection;
				if (GetPix > 0) {
					if (BackgroundPriority) {
						if (*DstPointer == ColourZero)
							*DstPointer = TranslatedPaletteObj[GetPix + PaletteOffset];
					}
					else *DstPointer = TranslatedPaletteObj[GetPix + PaletteOffset];
				}
                DstPointer++;
                PixX++;
			}
            
        }
        
    }
    
}

void GGBC::ReadLine_SGB() {

	// Get relevant parameters from status registers and such:
	const unsigned char LCDCtrl = IOPorts[0x40];
	const unsigned char LCDStatus = IOPorts[0x41];
	unsigned char ScrY = IOPorts[0x42];
	unsigned char ScrX = IOPorts[0x43];
	const unsigned int LineNo = IOPorts[0x44];

	// More variables
	unsigned int Offset, Max;
	unsigned int TileSetBase;
	unsigned int TileMapBase;
	unsigned int PixX, PixY, TileX, TileY;
	unsigned int TileNo;
	unsigned int* DstPointer;
	unsigned int* TilesetPointer;

	// Sprite-specific stuff:
	unsigned int Flags;
	unsigned int PaletteOffset;
	unsigned int PixInc;
	unsigned int GetPix;

	TileSetBase = LCDCtrl & 0x10 ? 0x0000 : 0x0100;
	TileMapBase = LCDCtrl & 0x08 ? 0x1c00 : 0x1800;

	// Check for BG enabled first:
	if (LCDCtrl & 0x01) {

		// Set point to draw to
		DstPointer = &SGB_MonoData[160 * LineNo];

		// Set starting point of VRAM data to read, in terms of pixel coordinates within the tile, and tile coordinates within the tilemap
		PixX = ScrX % 8;
		PixY = (LineNo + ScrY) % 8;
		TileX = ScrX / 8;
		TileY = ((LineNo + ScrY) % 256) / 8;

		// Draw first 20 tiles (including partial leftmost tile)
		for (Offset = 0; Offset < 20; Offset++) {

			// Get tile no. and point to the tileset data to read
			TileNo = VRAM[TileMapBase + 32 * TileY + TileX];
			if (TileNo < 127) TileNo += TileSetBase;
			TilesetPointer = &TileSet[TileNo * 64 + 8 * PixY + PixX];

			// Draw up to 8 pixels of this tile
			while (PixX < 8) {
				*DstPointer++ = SGB_TranslatePaletteBG[*TilesetPointer++];
				PixX++;
			}

			// Reset pixel counter and get next tile coordinates
			PixX = 0;
			TileX = (TileX + 1) % 32;

		}

		// Draw partial 21st tile. Find out how many pixels of it to draw.
		Max = ScrX % 8;

		// Get tile no
		TileNo = VRAM[TileMapBase + 32 * TileY + TileX];
		if (TileNo < 127) TileNo += TileSetBase;
		TilesetPointer = &TileSet[TileNo * 64 + 8 * PixY + PixX];

		// Draw up to 8 pixels of this tile
		while (PixX < Max) {
			*DstPointer++ = SGB_TranslatePaletteBG[*TilesetPointer++];
			PixX++;
		}

	}

	// Check for window enabled first, and check for window onscreen:
	ScrX = IOPorts[0x4b];
	ScrY = IOPorts[0x4a];
	TileMapBase = LCDCtrl & 0x40 ? 0x1c00 : 0x1800;
	if (((LCDCtrl & 0x20) != 0x00) && (ScrX < 167) && (ScrY <= LineNo)) {

		// Subtract 7 from window X pos
		if (ScrX > 6) ScrX -= 7;

		// Set point to draw to
		DstPointer = &SGB_MonoData[160 * LineNo + ScrX];

		// Set starting point of VRAM data to read, in terms of pixel coordinates within the tile, and tile coordinates within the tilemap
		PixX = 0;
		PixY = (LineNo - ScrY) % 8;
		TileX = 0;
		TileY = (LineNo - ScrY) / 8;

		// Check how many complete tiles to draw
		Max = (160 - ScrX) / 8;

		// Draw all the complete tiles until the end of the line
		for (Offset = 0; Offset < Max; Offset++) {

			// Get tile no
			TileNo = VRAM[TileMapBase + 32 * TileY + TileX];
			if (TileNo < 127) TileNo += TileSetBase;
			TilesetPointer = &TileSet[TileNo * 64 + 8 * PixY + PixX];

			// Draw the 8 pixels of this tile
			while (PixX < 8) {
				*DstPointer++ = SGB_TranslatePaletteBG[*TilesetPointer++];
				PixX++;
			}

			// Reset pixel counter and get next tile coordinates
			PixX = 0;
			TileX = (TileX + 1) % 32;

		}

		// Draw partial last tile. Find out how many pixels of it to draw.
		Max = ScrX % 8;

		// Get tile no
		TileNo = VRAM[TileMapBase + 32 * TileY + TileX];
		if (TileNo < 127) TileNo += TileSetBase;
		TilesetPointer = &TileSet[TileNo * 64 + 8 * PixY + PixX];

		// Draw up to 8 pixels of this tile
		while (PixX < Offset) {
			*DstPointer++ = SGB_TranslatePaletteBG[*TilesetPointer++];
			PixX++;
		}

	}

	// Check for sprites enabled first:
	if (LCDCtrl & 0x02) {

		const unsigned char LargeSprites = LCDCtrl & 0x04;

		// Draw each sprite that is visible
		for (Offset = 156; Offset < 160; Offset -= 4) {

			// Get coords check if off-screen
			ScrY = OAM[Offset];
			if (ScrY == 0) continue;
			if (ScrY > 159) continue;
			ScrX = OAM[Offset + 1];
			if (ScrX == 0) continue;
			if (ScrX > 167) continue;

			// Check if the current scan line would pass through an 8x16 sprite
			if (LineNo + 16 < ScrY) continue;
			if (LineNo >= ScrY) continue;
			TileNo = OAM[Offset + 2];
			Flags = OAM[Offset + 3];

			// Check sprite size, and accordingly re-assess visibility and adjust tilemap index
			if (LargeSprites) {
				if (LineNo + 8 >= ScrY) TileNo |= 0x01;
				else TileNo &= 0xfe;

			}
			else if (LineNo + 8 >= ScrY) continue;

			// Get pixel row within tile that will be drawn
			PixY = (LineNo + 16 - ScrY) % 8;

			// Set which palette to draw with
			PaletteOffset = Flags & 0x10 ? 4 : 0;
			GLuint ColourZero = TranslatedPaletteBG[0];

			// Get first pixel in tile row to draw, plus how many pixels to draw, and adjust the starting point to write to in the image
			if (ScrX < 8) {
				PixX = 8 - ScrX;
				Max = ScrX;
				ScrX = 0;
			}
			else if (ScrX > 160) {
				PixX = 0;
				Max = 168 - ScrX;
				ScrX -= 8;
			}
			else {
				PixX = 0;
				Max = 8;
				ScrX -= 8;
			}

			// Adjust X if horizontally flipping
			int TilesetPointerDirection;
			if (Flags & 0x20) {
				PixX = 7 - PixX;
				TilesetPointerDirection = -1;
			}
			else TilesetPointerDirection = 1;

			// Adjust Y if vertically flipping
			if (Flags & 0x40)
				PixY = 7 - PixY;

			// Get priority flag
			unsigned int BackgroundPriority = Flags & 0x80;

			// Set point to draw to
			DstPointer = &SGB_MonoData[160 * LineNo + ScrX];

			// Get pointer to tile data
			TilesetPointer = &TileSet[TileNo * 64 + 8 * PixY + PixX];

			// Draw up to 8 pixels of this tile (skipping over transparent pixels with palette index 0, or if obscured by the background)
			PixX = 0;
			while (PixX < Max) {
				GetPix = *TilesetPointer;
				TilesetPointer += TilesetPointerDirection;
				if (GetPix > 0) {
					if (BackgroundPriority) {
						if (*DstPointer == ColourZero)
							*DstPointer = SGB_TranslatePaletteObj[GetPix + PaletteOffset];
					}
					else *DstPointer = SGB_TranslatePaletteObj[GetPix + PaletteOffset];
				}
				DstPointer++;
				PixX++;
			}

		}

	}

}

void GGBC::ReadLine_GBC() {
    /* 
    // Get relevant parameters from staus registers and siuch:
    unsigned char LCDCtrl = IOPorts[0x40];
    unsigned char LCDStatus = IOPorts[0x41];
    unsigned char ScrY = IOPorts[0x42];
    unsigned char ScrX = IOPorts[0x43];
    unsigned int LineNo = IOPorts[0x44];

    // More variables:
    unsigned int tx, ty, px, py,
                 SprAddr,
                 dx, dy;
    unsigned int tx_max;
    unsigned int TMap_Base;
    unsigned int TSet_Base;
    unsigned char *DataPoint;
    unsigned int LastPixels, CountLastPixels;
    unsigned char Tile, Flags;
    unsigned int LargeSprites, XMirror, YMirror, SprPrio;
    GLfloat BGColourZero;
    unsigned char PaletteData;
    
    // Introduced for GBC
    unsigned int PalNo;
    unsigned int VRAMBankNo;

if (LineNo == 5)
    TempByte = 0x32;

    // Check if LCD is disabled (set all white if it is):
    TempByte = LCDCtrl & 0x80; // LCD enable flag
    TempByte2 = LCDCtrl & 0x23; // Window enable, sprite enable, and BG enable flags
    if (TempByte == 0x00 || TempByte2 == 0x00) {
        for (tx = 0; tx < 160; tx++) {
            imgData[LineNo][tx][0] = 1.0;
            imgData[LineNo][tx][1] = 1.0;
            imgData[LineNo][tx][2] = 1.0;
        }
        return;
    }

    TempByte = LCDCtrl & 0x10;
    if (TempByte == 0x00) TSet_Base = 0x1000; else TSet_Base = 0x0000;

    // Check for BG enabled first:
    TempByte = LCDCtrl & 0x01;
    if (TempByte != 0x00) {

        // Set variables before beginning:
        tx_max = (unsigned int)(ScrX / 8) + 21; // Max 21 tiles visible horizontally
        TempByte = LCDCtrl & 0x08;
        if (TempByte == 0x00) TMap_Base = 0x1800; else TMap_Base = 0x1c00;
    
        // Starting position of screen pixel coordinates
        px = 0 - (unsigned int)(ScrX % 8);
        py = LineNo;

        // Number of pixels to draw in partially ofscreen tiles at end of scanline
        LastPixels = (unsigned int)(ScrX % 8);
        if (LastPixels == 0) tx_max--; // Tiles are aligned exactly in x, so fewer tiles needed
    
        // Loop to draw background tiles:
        ty = ((unsigned int)ScrY + (unsigned int)LineNo) / 8;
        ty = ty % 32;
        for (tx = (unsigned int)(ScrX / 8); tx < tx_max; tx++) {

            // Get tile no. from the tile map
            TempAddr = TMap_Base + (tx % 32) + (ty * 32);
            PalNo = VRAM[1][TempAddr];
            if ((PalNo & 0x08) != 0) VRAMBankNo = 1;
            else VRAMBankNo = 0;
            Tile = VRAM[0][TempAddr];
            PalNo &= 0x07;

            // Find address of tile in set
            TempByte = Tile & 0x80;
            if (TSet_Base != 0 && TempByte != 0x00) { // Signed tile no's used in this set
                TempByte = ~Tile + 1;
                TempAddr = TSet_Base - (unsigned int)(TempByte)*16;
            }
            else
                TempAddr = TSet_Base + (unsigned int)(Tile)*16;

            // Point to pixel data for this row
            DataPoint = mem.TileSet[TempAddr/2 + ((ScrY + LineNo) % 8) + VRAMBankNo*3072];

            // Write to final pixel data
            if (px < 153) {
                imgData[py][px][0] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][0];
                imgData[py][px][1] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][1];
                imgData[py][px++][2] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint++)][2];
                imgData[py][px][0] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][0];
                imgData[py][px][1] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][1];
                imgData[py][px++][2] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint++)][2];
                imgData[py][px][0] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][0];
                imgData[py][px][1] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][1];
                imgData[py][px++][2] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint++)][2];
                imgData[py][px][0] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][0];
                imgData[py][px][1] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][1];
                imgData[py][px++][2] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint++)][2];
                imgData[py][px][0] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][0];
                imgData[py][px][1] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][1];
                imgData[py][px++][2] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint++)][2];
                imgData[py][px][0] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][0];
                imgData[py][px][1] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][1];
                imgData[py][px++][2] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint++)][2];
                imgData[py][px][0] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][0];
                imgData[py][px][1] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][1];
                imgData[py][px++][2] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint++)][2];
                imgData[py][px][0] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][0];
                imgData[py][px][1] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][1];
                imgData[py][px++][2] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint++)][2];
            }
            else if (px < 160) {
                for (CountLastPixels = 0; CountLastPixels < LastPixels; CountLastPixels++) {
                    imgData[py][px][0] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][0];
                    imgData[py][px][1] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][1];
                    imgData[py][px++][2] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint++)][2];
                }
            }
            else {
                px = 0;
                DataPoint += LastPixels;
                for (CountLastPixels = 0; CountLastPixels < 8 - LastPixels; CountLastPixels++) {
                    imgData[py][px][0] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][0];
                    imgData[py][px][1] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][1];
                    imgData[py][px++][2] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint++)][2];
                }
            }

        } // Next tile in row
    } // End if (BG enabled test)

    // Check for window enabled first
    TempByte = LCDCtrl & 0x20;
    ScrX = IOPorts[0x4b];
    ScrY = IOPorts[0x4a];
    if ((TempByte != 0x00) && (ScrX < 167) && (ScrY <= LineNo)) {

        // Set variables before beginning:
        tx_max = 21 - (unsigned int)(ScrX / 8); // Max 21 tiles visible horizontally
        ScrX -= 7;
        TempByte = LCDCtrl & 0x40;
        if (TempByte == 0x00) TMap_Base = 0x1800; else TMap_Base = 0x1c00;
        px = ScrX;

        // Starting position of screen pixel coordinates
        px = 0 - (unsigned int)(ScrX % 8);
        py = LineNo;

        // Number of pixels to draw in partially ofscreen tiles at end of scanline
        LastPixels = (unsigned int)(ScrX % 8);
        if (LastPixels == 0) tx_max--; // Tiles are aligned exactly in x, so fewer tiles needed
    
        // Loop to draw background tiles:
        ty = ((unsigned int)LineNo - (unsigned int)ScrY) / 8;
        ty = ty % 32;
        for (tx = (unsigned int)(ScrX / 8); tx < tx_max; tx++) {

            // Get tile no. from the tile map
            TempAddr = TMap_Base + (tx % 32) + (ty * 32);
            Tile = VRAM[0][TempAddr];
            PalNo = VRAM[1][TempAddr];
            if ((PalNo & 0x08) != 0) VRAMBankNo = 1;
            else VRAMBankNo = 0;
            PalNo &= 0x07;

            // Find address of tile in set
            TempByte = Tile & 0x80;
            if (TSet_Base != 0 && TempByte != 0x00) { // Signed tile no's used in this set
                TempByte = ~Tile + 1;
                TempAddr = TSet_Base - (unsigned int)(TempByte)*16;
            }
            else
                TempAddr = TSet_Base + (unsigned int)(Tile)*16;

            // Point to pixel data for this row
            DataPoint = mem.TileSet[TempAddr/2 + ((LineNo - ScrY) % 8) + VRAMBankNo*3072];

            // Write to final pixel data
            if (px < 153) {
                imgData[py][px][0] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][0];
                imgData[py][px][1] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][1];
                imgData[py][px++][2] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint++)][2];
                imgData[py][px][0] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][0];
                imgData[py][px][1] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][1];
                imgData[py][px++][2] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint++)][2];
                imgData[py][px][0] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][0];
                imgData[py][px][1] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][1];
                imgData[py][px++][2] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint++)][2];
                imgData[py][px][0] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][0];
                imgData[py][px][1] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][1];
                imgData[py][px++][2] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint++)][2];
                imgData[py][px][0] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][0];
                imgData[py][px][1] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][1];
                imgData[py][px++][2] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint++)][2];
                imgData[py][px][0] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][0];
                imgData[py][px][1] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][1];
                imgData[py][px++][2] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint++)][2];
                imgData[py][px][0] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][0];
                imgData[py][px][1] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][1];
                imgData[py][px++][2] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint++)][2];
                imgData[py][px][0] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][0];
                imgData[py][px][1] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][1];
                imgData[py][px++][2] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint++)][2];
            }
            else if (px < 160) {
                for (CountLastPixels = 0; CountLastPixels < LastPixels; CountLastPixels++) {
                    imgData[py][px][0] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][0];
                    imgData[py][px][1] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][1];
                    imgData[py][px++][2] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint++)][2];
                }
            }
            else {
                px = 0;
                DataPoint += LastPixels;
                for (CountLastPixels = 0; CountLastPixels < 8 - LastPixels; CountLastPixels++) {
                    imgData[py][px][0] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][0];
                    imgData[py][px][1] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint)][1];
                    imgData[py][px++][2] = mem.CGB_BGPalette[PalNo][(unsigned int)*(DataPoint++)][2];
                }
            }

        } // Next tile in row
    } // End if (window enabled test)

    // Check for sprites enabled first
    TempByte = LCDCtrl & 0x02;
    if (TempByte != 0x00) {

        if ((LCDCtrl & 0x04) != 0x00)
            LargeSprites = 1;
        else
            LargeSprites = 0;

        // Loop through 40 sprites
        for (SprAddr = 156; SprAddr < 160; SprAddr -= 4) {
            
            // Get sprite attributes
            px = mem.SAT[SprAddr + 1];
            py = mem.SAT[SprAddr];
            // Test if offscreen
            px--;
            py--;
            if (px > 167)
                continue;
            if (py > 159)
                continue;
            px++;
            py++;
            // Test if current line passes through (or not)
            if (py > LineNo + 16)
                continue;
            if (LargeSprites) {
                if (py <= LineNo)
                    continue;
            }
            else {
                if (py <= LineNo + 8)
                    continue;
            }
            px -= 8;
            py -= 16;
            Tile = mem.SAT[SprAddr + 2];
            Flags = mem.SAT[SprAddr + 3];

            // Check for mirroring:
            if ((Flags & 0x20) != 0) XMirror = 1; else XMirror = 0;
            if ((Flags & 0x40) != 0) YMirror = 1; else YMirror = 0;

            if (XMirror)
                px += 7;

            // Get VRAM bank and palette no. from flags
            VRAMBankNo = (Flags & 0x08) >> 3;
            PalNo = Flags & 0x07;

            // Set priority to sprites or BG:
            if ((Flags & 0x80) == 0x00) SprPrio = 1;
            else SprPrio = 0;

            // Check for large sprites, and set tile accordingly:
            if (LargeSprites) { // 8x8 situation
                if (YMirror) {
                    if (LineNo > py + 7) // 1st tile
                        Tile = Tile & 0xfe;
                    else // 2nd tile
                        Tile = Tile | 0x01;
                }
                else {
                    if (LineNo <= py + 7) // 1st tile
                        Tile = Tile & 0xfe;
                    else // 2nd tile
                        Tile = Tile | 0x01;
                }
            }

            // Find address of tile in set
            TempAddr = (unsigned int)(Tile)*16;

            // Check for y mirroring, and set row of pixels to draw
            if (YMirror) {
                if (LargeSprites)
                    DataPoint = mem.TileSet[TempAddr/2 + ((py + 15 - LineNo) % 8) + VRAMBankNo*3072];
                else
                    DataPoint = mem.TileSet[TempAddr/2 + (py + 7 - LineNo) + VRAMBankNo*3072];
            }
            else {
                if (LargeSprites)
                    DataPoint = mem.TileSet[TempAddr/2 + ((LineNo - py) % 8) + VRAMBankNo*3072];
                else
                    DataPoint = mem.TileSet[TempAddr/2 + (LineNo - py) + VRAMBankNo*3072];
            }

            // Write to final pixel data
            if (SprPrio) {
                if (XMirror) {
                    if ((px < 160) && (px >= 7)) {
                        if (DataPoint[0] != 0) {
                            imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[0])][0];
                            imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[0])][1];
                            imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[0])][2];
                        } px--;
                        if (DataPoint[1] != 0) {
                            imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[1])][0];
                            imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[1])][1];
                            imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[1])][2];
                        } px--;
                        if (DataPoint[2] != 0) {
                            imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[2])][0];
                            imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[2])][1];
                            imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[2])][2];
                        } px--;
                        if (DataPoint[3] != 0) {
                            imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[3])][0];
                            imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[3])][1];
                            imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[3])][2];
                        } px--;
                        if (DataPoint[4] != 0) {
                            imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[4])][0];
                            imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[4])][1];
                            imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[4])][2];
                        } px--;
                        if (DataPoint[5] != 0) {
                            imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[5])][0];
                            imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[5])][1];
                            imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[5])][2];
                        } px--;
                        if (DataPoint[6] != 0) {
                            imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[6])][0];
                            imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[6])][1];
                            imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[6])][2];
                        } px--;
                        if (DataPoint[7] != 0) {
                            imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[7])][0];
                            imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[7])][1];
                            imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[7])][2];
                        }
                    }
                    else if ((px < 167) && (px >= 7)) {
                        LastPixels = (167 - px) % 8;
                        px = 159;
                        for (CountLastPixels = 0; CountLastPixels < LastPixels; CountLastPixels++) {
                            if (DataPoint[CountLastPixels] != 0) {
                                imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[8-LastPixels+CountLastPixels])][0];
                                imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[8-LastPixels+CountLastPixels])][1];
                                imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[8-LastPixels+CountLastPixels])][2];
                            }
                            px--;
                        }
                    }
                    else if (px >= 0) {
                        LastPixels = (px + 1) % 8;
                        for (CountLastPixels = 0; CountLastPixels < LastPixels; CountLastPixels++) {
                            if (DataPoint[CountLastPixels] != 0) {
                                imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[CountLastPixels])][0];
                                imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[CountLastPixels])][1];
                                imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[CountLastPixels])][2];
                            }
                            px--;
                        }
                    }
                }
                else {
                    if ((px < 153) && (px >= 0)) {
                        if (DataPoint[0] != 0) {
                            imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[0])][0];
                            imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[0])][1];
                            imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[0])][2];
                        } px++;
                        if (DataPoint[1] != 0) {
                            imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[1])][0];
                            imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[1])][1];
                            imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[1])][2];
                        } px++;
                        if (DataPoint[2] != 0) {
                            imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[2])][0];
                            imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[2])][1];
                            imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[2])][2];
                        } px++;
                        if (DataPoint[3] != 0) {
                            imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[3])][0];
                            imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[3])][1];
                            imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[3])][2];
                        } px++;
                        if (DataPoint[4] != 0) {
                            imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[4])][0];
                            imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[4])][1];
                            imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[4])][2];
                        } px++;
                        if (DataPoint[5] != 0) {
                            imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[5])][0];
                            imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[5])][1];
                            imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[5])][2];
                        } px++;
                        if (DataPoint[6] != 0) {
                            imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[6])][0];
                            imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[6])][1];
                            imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[6])][2];
                        } px++;
                        if (DataPoint[7] != 0) {
                            imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[7])][0];
                            imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[7])][1];
                            imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[7])][2];
                        }
                    }
                    else if ((px < 160) && (px >= 0)) {
                        LastPixels = (160 - (unsigned int)px) % 8;
                        for (CountLastPixels = 0; CountLastPixels < LastPixels; CountLastPixels++) {
                            if (DataPoint[CountLastPixels] != 0) {
                                imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[CountLastPixels])][0];
                                imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[CountLastPixels])][1];
                                imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[CountLastPixels])][2];
                            }
                            px++;
                        }
                    }
                    else if (px >= 0) {
                        LastPixels = (px + 8) % 8;
                        px = 0;
                        for (CountLastPixels = 0; CountLastPixels < 8 - LastPixels; CountLastPixels++) {
                            if (DataPoint[CountLastPixels] != 0) {
                                imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[8-LastPixels+CountLastPixels])][0];
                                imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[8-LastPixels+CountLastPixels])][1];
                                imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[8-LastPixels+CountLastPixels])][2];
                            }
                            px++;
                        }
                    }
                }
            }
            else { // BG has priority
                if (XMirror) {
                    if ((px < 160) && (px >= 7)) {
                        if (DataPoint[0] != 0) {
                            if (imgData[LineNo][px][2] == BGColourZero) {
                                imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[0])][0];
                                imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[0])][1];
                                imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[0])][2];
                            }
                        } px--;
                        if (DataPoint[1] != 0) {
                            if (imgData[LineNo][px][2] == BGColourZero) {
                                imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[1])][0];
                                imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[1])][1];
                                imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[1])][2];
                            }
                        } px--;
                        if (DataPoint[2] != 0) {
                            if (imgData[LineNo][px][2] == BGColourZero) {
                                imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[2])][0];
                                imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[2])][1];
                                imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[2])][2];
                            }
                        } px--;
                        if (DataPoint[3] != 0) {
                            if (imgData[LineNo][px][2] == BGColourZero) {
                                imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[3])][0];
                                imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[3])][1];
                                imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[3])][2];
                            }
                        } px--;
                        if (DataPoint[4] != 0) {
                            if (imgData[LineNo][px][2] == BGColourZero) {
                                imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[4])][0];
                                imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[4])][1];
                                imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[4])][2];
                            }
                        } px--;
                        if (DataPoint[5] != 0) {
                            if (imgData[LineNo][px][2] == BGColourZero) {
                                imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[5])][0];
                                imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[5])][1];
                                imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[5])][2];
                            }
                        } px--;
                        if (DataPoint[6] != 0) {
                            if (imgData[LineNo][px][2] == BGColourZero) {
                                imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[6])][0];
                                imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[6])][1];
                                imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[6])][2];
                            }
                        } px--;
                        if (DataPoint[7] != 0) {
                            if (imgData[LineNo][px][2] == BGColourZero) {
                                imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[7])][0];
                                imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[7])][1];
                                imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[7])][2];
                            }
                        }
                    }
                    else if ((px < 167) && (px >= 7)) {
                        LastPixels = (167 - px) % 8;
                        px = 159;
                        for (CountLastPixels = 0; CountLastPixels < LastPixels; CountLastPixels++) {
                            if (DataPoint[CountLastPixels] != 0) {
                                if (imgData[LineNo][px][2] == BGColourZero) {
                                    imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[8-LastPixels+CountLastPixels])][0];
                                    imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[8-LastPixels+CountLastPixels])][1];
                                    imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[8-LastPixels+CountLastPixels])][2];
                                }
                            }
                            px--;
                        }
                    }
                    else if (px >= 0) {
                        LastPixels = (px + 1) % 8;
                        for (CountLastPixels = 0; CountLastPixels < LastPixels; CountLastPixels++) {
                            if (DataPoint[CountLastPixels] != 0) {
                                if (imgData[LineNo][px][2] == BGColourZero) {
                                    imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[CountLastPixels])][0];
                                    imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[CountLastPixels])][1];
                                    imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[CountLastPixels])][2];
                                }
                            }
                            px--;
                        }
                    }
                }
                else {
                    if ((px < 153) && (px >= 0)) {
                        if (DataPoint[0] != 0) {
                            if (imgData[LineNo][px][2] == BGColourZero) {
                                imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[0])][0];
                                imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[0])][1];
                                imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[0])][2];
                            }
                        } px++;
                        if (DataPoint[1] != 0) {
                            if (imgData[LineNo][px][2] == BGColourZero) {
                                imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[1])][0];
                                imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[1])][1];
                                imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[1])][2];
                            }
                        } px++;
                        if (DataPoint[2] != 0) {
                            if (imgData[LineNo][px][2] == BGColourZero) {
                                imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[2])][0];
                                imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[2])][1];
                                imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[2])][2];
                            }
                        } px++;
                        if (DataPoint[3] != 0) {
                            if (imgData[LineNo][px][2] == BGColourZero) {
                                imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[3])][0];
                                imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[3])][1];
                                imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[3])][2];
                            }
                        } px++;
                        if (DataPoint[4] != 0) {
                            if (imgData[LineNo][px][2] == BGColourZero) {
                                imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[4])][0];
                                imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[4])][1];
                                imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[4])][2];
                            }
                        } px++;
                        if (DataPoint[5] != 0) {
                            if (imgData[LineNo][px][2] == BGColourZero) {
                                imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[5])][0];
                                imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[5])][1];
                                imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[5])][2];
                            }
                        } px++;
                        if (DataPoint[6] != 0) {
                            if (imgData[LineNo][px][2] == BGColourZero) {
                                imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[6])][0];
                                imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[6])][1];
                                imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[6])][2];
                            }
                        } px++;
                        if (DataPoint[7] != 0) {
                            if (imgData[LineNo][px][2] == BGColourZero) {
                                imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[7])][0];
                                imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[7])][1];
                                imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[7])][2];
                            }
                        }
                    }
                    else if ((px < 160) && (px >= 0)) {
                        LastPixels = (160 - (unsigned int)px) % 8;
                        for (CountLastPixels = 0; CountLastPixels < LastPixels; CountLastPixels++) {
                            if (DataPoint[CountLastPixels] != 0) {
                                if (imgData[LineNo][px][2] == BGColourZero) {
                                    imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[CountLastPixels])][0];
                                    imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[CountLastPixels])][1];
                                    imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[CountLastPixels])][2];
                                }
                            }
                            px++;
                        }
                    }
                    else if (px >= 0) {
                        LastPixels = (px + 8) % 8;
                        px = 0;
                        for (CountLastPixels = 0; CountLastPixels < 8 - LastPixels; CountLastPixels++) {
                            if (DataPoint[CountLastPixels] != 0) {
                                if (imgData[LineNo][px][2] == BGColourZero) {
                                    imgData[LineNo][px][0] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[8-LastPixels+CountLastPixels])][0];
                                    imgData[LineNo][px][1] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[8-LastPixels+CountLastPixels])][1];
                                    imgData[LineNo][px][2] = mem.CGB_OBJPalette[PalNo][(unsigned int)(DataPoint[8-LastPixels+CountLastPixels])][2];
                                }
                            }
                            px++;
                        }
                    }
                }
            }
        }
    }
    */
}

void GGBC::SGB_ColouriseFrame() {
    
    unsigned int px, py;
    unsigned int chrx, chry;
    unsigned int x_max, y_max;

	unsigned int PaletteNumber;
    GLuint* UsePalette;

    for (chrx = 0; chrx < 20; chrx++) {
        for (chry = 0; chry < 18; chry++) {

			PaletteNumber = SGB_CHRPalettes[chry * 20 + chrx];
			UsePalette = &SGB_Palettes[PaletteNumber * 4];

            x_max = 8 * chrx + 8;
            y_max = 8 * chry + 8;
            for (px = 8 * chrx; px < x_max; px++) {
                for (py = 8 * chry; py < y_max; py++)
                    ImgData[py * 160 + px] = UsePalette[SGB_MonoData[160 * py + px]];
            }

        }
    }
    
}
