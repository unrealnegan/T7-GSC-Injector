#include "../include/main.h"

s32 scePadReadState_hook(s32 handle, OrbisPadData* pad_data)
{
    s32 ret = 0;
    ret = scePadReadStateExt(handle, pad_data);
    if (ret)
		return ret;
	if (pad_data->buttons == PAD_BUTTON_TOGGLE_INPUTS && !g_busy)
	{
		g_busy = 1;
		g_inputs = !g_inputs;
		if (!g_inputs)
			snprintf(g_notif_str, sizeof(g_notif_str), "Inputs disabled");
		else
			snprintf(g_notif_str, sizeof(g_notif_str), "Inputs enabled");
		notif(TEX_ICON_CHAMPIONS_LEAGUE, g_notif_str);
		sleep(BUSY_INTERRUPT_S);
		g_busy = 0;
	}	
	if (pad_data->buttons == PAD_BUTTON_SHOW_MESSAGE && !g_busy && g_inputs)
	{
		g_busy = 1;
		snprintf(g_notif_str, sizeof(g_notif_str), "Original by 0x1496FD0\nModified by unrealnegan");
		notif(TEX_ICON_CHAMPIONS_LEAGUE, g_notif_str);
		sleep(BUSY_INTERRUPT_S);
		g_busy = 0;
	}
	if (!g_busy && g_inputs)
	{
		switch (pad_data->buttons)
		{
			case PAD_BUTTON_INJECT:
				g_busy = 1;
				inject();
				break;
			case PAD_BUTTON_NEXT_GSC:
				g_busy = 1;
				switch_gsc(1);
				break;
			case PAD_BUTTON_PREV_GSC:
				g_busy = 1;
				switch_gsc(0);
				break;
		}
	}
	return ret;
}

s32 attr_public plugin_load(s32 argc, const char* argv[])
{
	snprintf(g_gsc_dir_str, sizeof(g_gsc_dir_str), "/data/ModMenuInjector");
	if (!dir_or_file_exist(g_gsc_dir_str, 0))
		sceKernelMkdir(g_gsc_dir_str, 0777);

	char module[256];
    snprintf(module, sizeof(module), "/%s/common/lib/%s", sceKernelGetFsSandboxRandomWord(), "libScePad.sprx");
    s32 libScePad = 0;
    sys_dynlib_load_prx(module, &libScePad);
	if (!libScePad)
	{
		snprintf(g_notif_str, sizeof(g_notif_str), "Failed, error code 0%d", ++g_errcode);
		notif(TEX_ICON_CHAMPIONS_LEAGUE, g_notif_str);
		return 0;
	}

	g_pshellcode = (Patcher*)malloc(sizeof(Patcher));
    Patcher_Construct(g_pshellcode);

    u8 xor_edx_edx[5] = { 0x31, 0xD2, 0x90, 0x90, 0x90 };
    Patcher_Install_Patch(g_pshellcode, (u64)scePadReadStateExt, xor_edx_edx, sizeof(xor_edx_edx));
	
	HOOK32(scePadReadState);

	snprintf(g_notif_str, sizeof(g_notif_str), "Mod Menu Loaded");
	notif(TEX_ICON_CHAMPIONS_LEAGUE, g_notif_str);
	sleep(1);
	snprintf(g_notif_str, sizeof(g_notif_str), "Original by 0x1496FD0\nModified by unrealnegan");
	notif(TEX_ICON_CHAMPIONS_LEAGUE, g_notif_str);
	sleep(1);
	snprintf(g_notif_str, sizeof(g_notif_str), "L1 + R3: Load menu\nL1 + Left/Right: Select menu\nL1 + Options: Toggle inputs\nL1 + Triangle: Show Credits");
	notif(TEX_ICON_CHAMPIONS_LEAGUE, g_notif_str);

	return 0;
}

s32 attr_public plugin_unload(s32 argc, const char* argv[])
{
	UNHOOK(scePadReadState);
	Patcher_Destroy(g_pshellcode);
    free(g_pshellcode);
	if (g_script_parse_tree.buffer)
		free(g_script_parse_tree.buffer);
	
	return 0;
}

s32 attr_module_hidden module_start(s64 argc, const void *args)
{
    return 0;
}

s32 attr_module_hidden module_stop(s64 argc, const void *args)
{
    return 0;
}

static void get_gsc_display_name(const char* path, char* display_name, size_t display_name_size)
{
	const char* filename = strrchr(path, '/');
	size_t filename_length;

	filename = filename ? filename + 1 : path;
	snprintf(display_name, display_name_size, "%s", filename);
	filename_length = strlen(display_name);
	if (filename_length >= 5 && strcmp(display_name + filename_length - 5, ".gscc") == 0)
		display_name[filename_length - 5] = '\0';
}

v query_files(const char* dir_path)
{
	if (g_files_count)
		for (u8 i = 0; i < g_files_count; i++)
			free(g_files_strarr[i]);
	g_files_count = get_files_from_dir(dir_path, g_files_strarr, TRUE, 0);
}

v inject()
{
	g_errcode = 0;

	mem_read(&g_script_parse_tree, g_offsets.script_parse_tree, sizeof(struct ScriptParseTree));
	u64 buffer;
	mem_read(&buffer, g_offsets.script_parse_tree + g_offsets.script_parse_tree_buffer, sizeof(buffer));
	s32 crc32;
	mem_read(&crc32, buffer + g_offsets.script_parse_tree_buffer_crc32, sizeof(crc32));

	query_files(g_gsc_dir_str);
	if (!g_files_count)
	{
		snprintf(g_notif_str, sizeof(g_notif_str), "No file detected ?");
		notif(TEX_ICON_CHAMPIONS_LEAGUE, g_notif_str);
		return;
	}

	if (strlen(g_current_gsc_str) == 0)
		snprintf(g_current_gsc_str, sizeof(g_current_gsc_str), "%s", g_files_strarr[0]);
	
	s32 file_size = get_file_size(g_current_gsc_str);
	if (file_size == -1)
	{
		snprintf(g_notif_str, sizeof(g_notif_str), "Loading failed, error code 0%d", ++g_errcode);
		notif(TEX_ICON_CHAMPIONS_LEAGUE, g_notif_str);
		return;
	}

	FILE* file = fopen(g_current_gsc_str, "r");
	if (file == NULL)
	{
		snprintf(g_notif_str, sizeof(g_notif_str), "Loading failed, error code 0%d", ++g_errcode);
		notif(TEX_ICON_CHAMPIONS_LEAGUE, g_notif_str);
		return;
	}

	s32 new_length = file_size;
		
	g_script_parse_tree.buffer = malloc(new_length + 1);
	g_script_parse_tree.length = new_length;

	if (!g_script_parse_tree.buffer)
	{
		fclose(file);
		snprintf(g_notif_str, sizeof(g_notif_str), "Loading failed, error code 0%d", ++g_errcode);
		notif(TEX_ICON_CHAMPIONS_LEAGUE, g_notif_str);
		return;
	}

	s32 bytes_read = fread((p)g_script_parse_tree.buffer, 1, file_size, file);
	if (bytes_read != file_size)
	{
    	free((p)g_script_parse_tree.buffer);
    	fclose(file);
    	snprintf(g_notif_str, sizeof(g_notif_str), "Loading failed, error code 0%d", ++g_errcode);
    	notif(TEX_ICON_CHAMPIONS_LEAGUE, g_notif_str);
    	return;
	}

	fclose(file);

	mem_write(&crc32, (u64)g_script_parse_tree.buffer + g_offsets.script_parse_tree_buffer_crc32, sizeof(crc32));
	mem_write(&g_script_parse_tree, g_offsets.script_parse_tree, sizeof(struct ScriptParseTree));

	char display_name[256];
	get_gsc_display_name(g_current_gsc_str, display_name, sizeof(display_name));
	snprintf(g_notif_str, sizeof(g_notif_str), "%s Loaded", display_name);
    notif(TEX_ICON_CHAMPIONS_LEAGUE, g_notif_str);

	sleep(BUSY_INTERRUPT_S);
	g_busy = 0;
}

v switch_gsc(u8 next_or_prev)
{
	char display_name[256];

	query_files(g_gsc_dir_str);
	if (!g_files_count)
	{
		snprintf(g_notif_str, sizeof(g_notif_str), "No file detected ?");
		notif(TEX_ICON_CHAMPIONS_LEAGUE, g_notif_str);
		return;
	}
	switch (next_or_prev)
    {
        case 1:
			if (g_current_gsc_index + 1 < g_files_count)
				g_current_gsc_index++;
			else
				g_current_gsc_index = 0;
        	strcpy(g_current_gsc_str, g_files_strarr[g_current_gsc_index]);
			break;
        default:
            if (g_current_gsc_index > 0 && g_current_gsc_index < g_files_count)
                g_current_gsc_index--;
            else
                g_current_gsc_index = g_files_count - 1;
			strcpy(g_current_gsc_str, g_files_strarr[g_current_gsc_index]);
			break;
    }

	get_gsc_display_name(g_current_gsc_str, display_name, sizeof(display_name));
	snprintf(g_notif_str, sizeof(g_notif_str), "%s", display_name);

	notif(TEX_ICON_CHAMPIONS_LEAGUE, g_notif_str);

	sleep(BUSY_INTERRUPT_S);
	g_busy = 0;
}
