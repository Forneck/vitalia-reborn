/* Copyright (c) 2018 castillo7@hotmail.com

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */

#define ASCRIPT(name) int name(char **str, struct char_data *self, \
                                           struct char_data *vict, \
                                           struct obj_data  *ovict, \
                                           int    from, \
                                           int    param)
struct str_script {
   char *name;
   void *function;   
   int  param;
};

int perform_script (char *str, struct char_data *self,
                               struct char_data *vict,
                               struct obj_data  *ovict,
                    int   from,
                    int   param);
