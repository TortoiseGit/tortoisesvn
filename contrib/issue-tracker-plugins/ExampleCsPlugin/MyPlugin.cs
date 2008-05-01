using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace ExampleCsPlugin
{
    [ComVisible(true),
        Guid("5870B3F1-8393-4c83-ACED-1D5E803A4F2B"),
        ClassInterface(ClassInterfaceType.None)]
    public class MyPlugin : Interop.BugTraqProvider.IBugTraqProvider
    {
        public bool ValidateParameters(IntPtr hParentWnd, string parameters)
        {
            return true;
        }

        public string GetLinkText(IntPtr hParentWnd, string parameters)
        {
            return "Choose Issue";
        }

        public string GetCommitMessage(IntPtr hParentWnd, string parameters, string commonRoot, string[] pathList,
                                       string originalMessage)
        {
            try
            {
                List<TicketItem> tickets = new List<TicketItem>();
                tickets.Add(new TicketItem(12, "Service doesn't start on Windows Vista"));
                tickets.Add(new TicketItem(19, "About box doesn't render correctly in large fonts mode"));

/*
                tickets.Add(new TicketItem(88, commonRoot));
                foreach (string path in pathList)
                    tickets.Add(new TicketItem(99, path));
 */

                MyIssuesForm form = new MyIssuesForm(tickets);
                if (form.ShowDialog() != DialogResult.OK)
                    return originalMessage;

                StringBuilder result = new StringBuilder(originalMessage);
                if (originalMessage.Length != 0 && !originalMessage.EndsWith("\n"))
                    result.AppendLine();

                foreach (TicketItem ticket in form.TicketsFixed)
                {
                    result.AppendFormat("Fixed #{0}: {1}", ticket.Number, ticket.Summary);
                    result.AppendLine();
                }

                return result.ToString();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());                
                throw;
            }
        }
    }
}
