#pragma once

#include <dirent.h>
#include "core/compat.h"

// store some state
struct dt_filebrowser_widget_t
{
  char cwd[PATH_MAX+100];  // current working directory
  struct dirent **ent;     // cached directory entries
  int ent_cnt;             // number of cached directory entries
  const char *selected;    // points to selected file name ent->d_name
};

// no need to explicitly call it, just sets 0
inline void
dt_filebrowser_init(
    dt_filebrowser_widget_t *w)
{
  memset(w, 0, sizeof(*w));
}

// this makes sure all memory is freed and also that the directory is re-read for new cwd.
inline void
dt_filebrowser_cleanup(
    dt_filebrowser_widget_t *w)
{
  for(int i=0;i<w->ent_cnt;i++)
    free(w->ent[i]);
  free(w->ent);
  w->ent_cnt = 0;
  w->ent = 0;
  w->selected = 0;
}

// TODO: make one that sorts dirs first:
// int alphasort(const struct dirent **a, const struct dirent **b);

namespace {

int dt_filebrowser_filter_dir(const struct dirent *d)
{
  if(d->d_name[0] == '.' && d->d_name[1] != '.') 
    return 0; // filter out hidden files
  if( !is_dir(d->d_name) ) 
    return 0; // filter out non-dirs too
  return 1;
}

int dt_filebrowser_filter_file(const struct dirent *d)
{
  if(d->d_name[0] == '.' && d->d_name[1] != '.') return 0; // filter out hidden files
  return 1;
}

}

inline void
dt_filebrowser_open(
    dt_filebrowser_widget_t *w)
{
  ImGui::SetNextWindowSize(ImVec2(vkdt.state.center_wd*0.8, vkdt.state.center_ht*0.8), ImGuiCond_Always);
  ImGui::OpenPopup("select directory");
}

inline void
dt_filebrowser(
    dt_filebrowser_widget_t *w,
    const char               mode) // 'f' or 'd'
{
  if(w->cwd[0] == 0) w->cwd[0] = '/';
  if(!w->ent)
  { // no cached entries, scan directory:
    w->ent_cnt = scandir(w->cwd, &w->ent,
        mode == 'd' ?
        &dt_filebrowser_filter_dir :
        &dt_filebrowser_filter_file,
        alphasort);
    if(w->ent_cnt == -1)
    {
      w->ent = 0;
      w->ent_cnt = 0;
    }
  }

  // display list of file names
  for(int i=0;i<w->ent_cnt;i++)
  {
    char name[260];
    snprintf(name, sizeof(name), "%s %s",
        is_dir(w->ent[i]->d_name) ? "[d]":"[f]", w->ent[i]->d_name);
    int selected = w->ent[i]->d_name == w->selected;
    if(ImGui::Selectable(name, selected, ImGuiSelectableFlags_DontClosePopups))
    {
      w->selected = w->ent[i]->d_name; // mark as selected
      if(is_dir(w->ent[i]->d_name))
      { // directory clicked
        // change cwd by appending to the string..
        int len = strnlen(w->cwd, sizeof(w->cwd));
        char *c = w->cwd;
        if(!strcmp(w->ent[i]->d_name, ".."))
        { // go up one dir
          c += len;
          *(--c) = 0;
          while(c > w->cwd && *c != '/') *(c--) = 0;
        }
        else
        { // append dir name
          snprintf(c+len, sizeof(w->cwd)-len-1, "%s/", w->ent[i]->d_name);
        }
        // ..and then cleaning up the dirent cache
        for(int i=0;i<w->ent_cnt;i++)
          free(w->ent[i]);
        free(w->ent);
        w->ent_cnt = 0;
        w->ent = 0;
        w->selected = 0;
      }
    }
  }
}

// returns 0 if cancelled, or 1 if "ok" has been pressed
inline int
dt_filebrowser_display(
    dt_filebrowser_widget_t *w,
    const char               mode) // 'f' or 'd'
{
  int ret = 0;
  if(ImGui::BeginPopupModal("select directory", 0, 0))
  {
    ImGui::PushID(w);
    ImGui::BeginChild("dirlist", ImVec2(0, vkdt.state.center_ht*0.75), 0);
    dt_filebrowser(w, mode);
    ImGui::EndChild();
    int wd = ImGui::GetWindowWidth()*0.496;
    int escidx = ImGui::GetIO().KeyMap[ImGuiKey_Escape];
    if(ImGui::Button("cancel", ImVec2(wd, 0)) || ImGui::IsKeyPressed(escidx))
      ImGui::CloseCurrentPopup();

    ImGui::SameLine();
    if(ImGui::Button("ok", ImVec2(wd, 0)))
    {
      ret = 1;
      ImGui::CloseCurrentPopup();
    }
    ImGui::PopID();
    ImGui::EndPopup();
  }
  return ret;
}
