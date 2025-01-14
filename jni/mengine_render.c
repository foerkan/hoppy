#define PushRenderCommandCase(type) \
	Assert((Commands->CapacityInBytes -\
	 		Commands->EntryAt +\
			sizeof(type)) >= 0);\
	type * Cmd = (type *)(Commands->Entries + Commands->EntryAt);\
	*Cmd = *((type *)Params);\
	Commands->EntryAt += sizeof(type);
//	Debug("After command : %d, %p", Commands->EntryAt, Cmd);
static void
PushRenderCommand(render_commands * Commands, 
				  render_command_entry_type Type, 
				  void * Params){
	BeginStackTraceBlock;
	/* TODO(furkan) : Make the render command buffer grow
		when its size is not large enough to store new command
	*/
	Assert((Commands->CapacityInBytes -
	 		Commands->EntryAt +
			sizeof(render_command_entry)) >= 0);
	render_command_entry * Entry = (render_command_entry *)(Commands->Entries + Commands->EntryAt);
//	Debug("Before : %d, %p", Commands->EntryAt, Entry);
	Entry->Type = Type;
	Commands->EntryAt += sizeof(render_command_entry);
//	Debug("After header : %d, %p", Commands->EntryAt, Entry);
	switch(Type){
		case RenderCommandEntryType_DrawBitmap:{
			PushRenderCommandCase(render_command_entry_drawbitmap);
		} break;
		case RenderCommandEntryType_DrawRect:{
//			Debug("Pushing a DrawRect command");
			PushRenderCommandCase(render_command_entry_drawrect);
		} break;
		case RenderCommandEntryType_DrawCircle:{
//			Debug("Pushing a DrawCircle command");
			PushRenderCommandCase(render_command_entry_drawcircle);
		} break;
		case RenderCommandEntryType_DrawString:{
//			Debug("Pushing a DrawString command");
			PushRenderCommandCase(render_command_entry_drawstring);
		} break;
		case RenderCommandEntryType_Clear:{
//			Debug("Pushing a Clear command");
			PushRenderCommandCase(render_command_entry_clear);
		} break;
		InvalidDefaultCase;
	}
	EndStackTraceBlock;
}

static void
ExtractGameRenderCommands(render_commands * RenderCommands,
							entity * EntitySentinel,
							asset_manager * AssetManager){
	BeginStackTraceBlock;

	entity * Entity = EntitySentinel->Prev;
	while(Entity != EntitySentinel){
		if(!Entity->IsEnabled || !Entity->IsVisible){
			Entity = Entity->Prev;
			continue;
		}

		switch(Entity->Type){
			case EntityType_Background:
			case EntityType_Enemy:
			case EntityType_Player: {
				render_command_entry_drawbitmap DrawBitmap;
				DrawBitmap.Position = Entity->Transform.Position;
				DrawBitmap.Size = Entity->Dimension;
				DrawBitmap.Bitmap = AssetManager->Bitmaps + 
										Entity->BitmapIndex;

				PushRenderCommand(RenderCommands,
					  RenderCommandEntryType_DrawBitmap,
					  &DrawBitmap);
#if 0	// NOTE(furkan) : Render collider bounds
				component_collider * Collider;
				Collider = (component_collider *)GetComponent(Entity, 
												ComponentType_Collider);
				
				if(Collider){
					if(Collider->Type == ColliderType_Rect){
						render_command_entry_drawrect DrawRect;
						DrawRect.Position = V2(Entity->Transform.Position.x
											+ Collider->Offset.x,
											Entity->Transform.Position.y
											+ Collider->Offset.y);
						DrawRect.Rect.Size = Collider->Size;
						DrawRect.Color = V4(0.8f, 0.6f, 0.0f, 0.5f);
						PushRenderCommand(RenderCommands,
										RenderCommandEntryType_DrawRect,
										&DrawRect);
					} else if(Collider->Type == ColliderType_Circle){
						render_command_entry_drawcircle DrawCircle;
						DrawCircle.Position = 
											V2(Entity->Transform.Position.x
											+ Collider->Offset.x,
											Entity->Transform.Position.y
											+ Collider->Offset.y);
						DrawCircle.Radius = Collider->Radius;			
						DrawCircle.Color = V4(1.0f, 0.5f, 0.0f, 0.5f);
						PushRenderCommand(RenderCommands,
										RenderCommandEntryType_DrawCircle,
										&DrawCircle);
					} else {
						Error("ExtractGameRenderCommands cannot render this type of collider : %d", Collider->Type);
					}
				}
#endif
			} break;
			InvalidDefaultCase;
		}

		Entity = Entity->Prev;
	}

	EndStackTraceBlock;
}

static void
ExtractRenderCommands(game_memory * Memory,
						render_commands * RenderCommands){
	BeginStackTraceBlock;
	/* NOTE(furkan) : Clear screen before rendering frames*/
	render_command_entry_clear ClearCommand;
	ClearCommand.Color.r = 0.8f;
	ClearCommand.Color.g = 0.0f;
	ClearCommand.Color.b = 0.6f;

	PushRenderCommand(RenderCommands, 
					  RenderCommandEntryType_Clear, 
					  &ClearCommand);	

	/*	TODO(furkan) : Introduce sorting order and 
		sort render commands accordingly */

	
	ui_controller * UI = &Memory->CurrentScreen->UIController;
	asset_manager * AssetManager = UI->AssetManager;
	switch(Memory->CurrentScreen->Type){
		case Screen_MainMenu:{
			u32 ElementIndex;
			for(ElementIndex=0; 
				ElementIndex < UI->ElementCount; 
				ElementIndex++){
				ui_element * Element = UI->Elements + ElementIndex;
			
				if(Element->IsEnabled){
					render_command_entry_drawbitmap DrawBitmap;
					DrawBitmap.Position = Element->Position;
					DrawBitmap.Size = Element->Rect.Size;
					DrawBitmap.Bitmap = AssetManager->Bitmaps + 
									Element->BitmapIndex[Element->State];

					PushRenderCommand(RenderCommands,
						  RenderCommandEntryType_DrawBitmap, &DrawBitmap);
				}
			}
		} break;
		case Screen_InGame:{
			entity * EntitySentinel = &Memory->EntitySentinel;
			ExtractGameRenderCommands(RenderCommands, EntitySentinel,
														AssetManager);
			u32 ElementIndex;
			for(ElementIndex=0; 
				ElementIndex < UI->ElementCount; 
				ElementIndex++){
				ui_element * Element = UI->Elements + ElementIndex;
				
				if(Element->IsEnabled){
					render_command_entry_drawbitmap DrawBitmap;
					DrawBitmap.Position = Element->Position;
					DrawBitmap.Size = Element->Rect.Size;
					DrawBitmap.Bitmap = AssetManager->Bitmaps + 
									Element->BitmapIndex[Element->State];

					PushRenderCommand(RenderCommands,
						  RenderCommandEntryType_DrawBitmap, &DrawBitmap);
				}
			}

			u32 Score = Memory->Score;
			u32 DigitCount;
			if(Score > 999999999){
				DigitCount = 10;
			} else if(Score > 99999999){
				DigitCount = 9;
			} else if(Score > 9999999){
				DigitCount = 8;
			} else if(Score > 999999){
				DigitCount = 7;
			} else if(Score > 99999){
				DigitCount = 6;
			} else if(Score > 9999){
				DigitCount = 5;
			} else if(Score > 999){
				DigitCount = 4;
			} else if(Score > 99){
				DigitCount = 3;
			} else if(Score > 9){
				DigitCount = 2;
			} else {
				DigitCount = 1;
			}

			render_command_entry_drawstring DrawString;
			DrawString.Position = V2(0.32f, 6.56f);
			DrawString.Color = V4(1.0f, 0.76f, 0.23f, 1.0f);
			DrawString.Height = 0.48f;
			DrawString.Font = Memory->Font;

			u32 CharIndex = DigitCount;
			DrawString.String[CharIndex--] = '\0';	
			u32 Val;
			while(1){
				DrawString.String[CharIndex--] = '0' + (Score % 10);
				Score /= 10;

				if(Score == 0 || CharIndex < 0){
					CharIndex++;
					break;
				}
			}

			PushRenderCommand(RenderCommands,
						  RenderCommandEntryType_DrawString, &DrawString);

		} break;
		case Screen_EndOfGame:{
			entity * EntitySentinel = &Memory->EntitySentinel;
			ExtractGameRenderCommands(RenderCommands, EntitySentinel,
														AssetManager);
			render_command_entry_drawrect DrawRect;
			DrawRect.Position = V2(6.40f, 3.60f);
			DrawRect.Rect.Size = V2(12.80, 7.20f);
			DrawRect.Color = V4(0.6f, 0.1098f, 0.2196f, 0.3f);
			PushRenderCommand(RenderCommands,
								RenderCommandEntryType_DrawRect,
								&DrawRect);

			u32 ElementIndex;
			for(ElementIndex=0; 
				ElementIndex < UI->ElementCount; 
				ElementIndex++){
				ui_element * Element = UI->Elements + ElementIndex;
				
				if(Element->IsEnabled){
					render_command_entry_drawbitmap DrawBitmap;
					DrawBitmap.Position = Element->Position;
					DrawBitmap.Size = Element->Rect.Size;
					DrawBitmap.Bitmap = AssetManager->Bitmaps + 
									Element->BitmapIndex[Element->State];

					PushRenderCommand(RenderCommands,
						  RenderCommandEntryType_DrawBitmap, &DrawBitmap);
				}
			}

			u32 Score = Memory->Score;
			u32 DigitCount;
			if(Score > 999999999){
				DigitCount = 10;
			} else if(Score > 99999999){
				DigitCount = 9;
			} else if(Score > 9999999){
				DigitCount = 8;
			} else if(Score > 999999){
				DigitCount = 7;
			} else if(Score > 99999){
				DigitCount = 6;
			} else if(Score > 9999){
				DigitCount = 5;
			} else if(Score > 999){
				DigitCount = 4;
			} else if(Score > 99){
				DigitCount = 3;
			} else if(Score > 9){
				DigitCount = 2;
			} else {
				DigitCount = 1;
			}

			render_command_entry_drawstring DrawString;
			DrawString.Position = V2(4.80f, 4.16f);
			DrawString.Color = V4(0.6f, 0.11f, 0.22f, 1.0f);
			DrawString.Height = 1.44f;
			DrawString.Font = Memory->Font;

			u32 CharIndex = DigitCount;
			DrawString.String[CharIndex--] = '\0';	
			u32 Val;
			while(1){
				DrawString.String[CharIndex--] = '0' + (Score % 10);
				Score /= 10;

				if(Score == 0 || CharIndex < 0){
					CharIndex++;
					break;
				}
			}

			PushRenderCommand(RenderCommands,
						  RenderCommandEntryType_DrawString, &DrawString);
		} break;
		InvalidDefaultCase;
	}

	EndStackTraceBlock;
}

static void 
SWRenderCommands(framebuffer * Framebuffer, render_commands * Commands){
	BeginStackTraceBlock;
	u8 * EntryAt;
	u8 * EntryAtLimit = ((u8* )Commands->Entries) + 
						Commands->CapacityInBytes;
	for(EntryAt = (u8 *)Commands->Entries; 
		EntryAt<EntryAtLimit;){
		render_command_entry * Entry = (render_command_entry *) EntryAt;
		EntryAt += sizeof(render_command_entry);
		switch(Entry->Type){
			case RenderCommandEntryType_DrawRect:{
				render_command_entry_drawrect * Command = (render_command_entry_drawrect *)EntryAt;
				EntryAt += sizeof(render_command_entry_drawrect);

				rect Rect = Command->Rect;
				if( Command->Position.y >= Framebuffer->Height ||
					Command->Position.x >= Framebuffer->Width || 
					Command->Position.y < 0 ||
					Command->Position.x < 0){
					/*	TODO(furkan) : This clipping must be done using 
						camera position
					*/
					/* NOTE(furkan) : Rect is out of the screen. Do not 
						waste time iterating over it */
					continue;
				}

				u32 * Row = ((u32 *) Framebuffer->Data) + (u32)(Framebuffer->Stride * (Framebuffer->Height - Command->Position.y));

				s32 RowLimit = Minimum(Command->Position.y+Rect.Size.Height, Framebuffer->Height);
				s32 ColLimit = Minimum(Command->Position.x+Rect.Size.Width,
										Framebuffer->Width);
				s32 RowAt;
				s32 ColAt;
				for(RowAt=Command->Position.y; 
					RowAt<RowLimit; 
					RowAt++){
					for(ColAt=Command->Position.x; 
						ColAt<ColLimit; 
						ColAt++){
						/* TODO(furkan) : Alpha blending!
						*/
						Row[ColAt] = //((u32)Rect.Color.a << 24) |
													(255 << 24) |
									 ((u32)Command->Color.b << 16) | 
									 ((u32)Command->Color.g << 8) | 
									  (u32)Command->Color.r;
					}

					Row -= Framebuffer->Stride;
				}
				
			} break;
			case RenderCommandEntryType_Clear:{
				render_command_entry_clear * Command = (render_command_entry_clear *)EntryAt;
				EntryAt += sizeof(render_command_entry_clear);
				
				/* TODO(furkan) : Alpha must be added and colors must be
					represented as v4
				*/
				u32 ClearColor = //((u32) Command->Color.a << 24) |
													(255 << 24) |
								 ((u32) Command->Color.b << 16) |
								 ((u32) Command->Color.g << 8) |
								 ((u32) Command->Color.r << 0);
				memset(Framebuffer->Data, ClearColor, Framebuffer->Size);
			}break;
			InvalidDefaultCase;
		}
	}
	EndStackTraceBlock;
}
